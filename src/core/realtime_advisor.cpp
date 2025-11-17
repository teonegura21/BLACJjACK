#include "realtime_advisor.hpp"
#include "../utils/logger.hpp"

namespace core {

RealtimeAdvisor::RealtimeAdvisor(
    const VisionConfig& visionConfig,
    const CountingConfig& countingConfig,
    const StrategyConfig& strategyConfig,
    const BettingConfig& bettingConfig)
    : m_visionConfig(visionConfig)
    , m_countingConfig(countingConfig)
    , m_strategyConfig(strategyConfig)
    , m_bettingConfig(bettingConfig)
{
}

RealtimeAdvisor::~RealtimeAdvisor() {
    shutdown();
}

bool RealtimeAdvisor::initialize() {
    auto& logger = utils::Logger::getInstance();
    logger.info("Initializing Realtime Advisor");

    // Initialize vision engine
    m_visionEngine = std::make_unique<vision::TensorRTEngine>(m_visionConfig);
    if (!m_visionEngine->loadSerializedEngine(m_visionConfig.model_path)) {
        logger.error("Failed to load TensorRT engine");
        return false;
    }
    m_visionEngine->warmup(10);
    logger.info("Vision engine initialized (avg inference: {:.2f}ms)",
                m_visionEngine->getAverageInferenceTime());

    // Initialize card counter
    m_cardCounter = std::make_unique<intelligence::CardCounter>();
    m_cardCounter->initialize(m_countingConfig.deck_count);
    logger.info("Card counter initialized ({} decks)", m_countingConfig.deck_count);

    // Initialize strategy engine
    m_basicStrategy = std::make_unique<intelligence::BasicStrategy>();
    m_basicStrategy->initialize(m_strategyConfig.basic_strategy_rules);
    logger.info("Strategy engine initialized ({})", m_strategyConfig.basic_strategy_rules);

    // Initialize betting strategy
    m_bettingStrategy = std::make_unique<intelligence::BettingStrategy>();
    m_bettingStrategy->configure(
        m_bettingConfig.min_bet,
        m_bettingConfig.max_bet,
        m_bettingConfig.kelly_fraction);
    m_bettingStrategy->setBankroll(10000.0); // Default bankroll
    logger.info("Betting strategy initialized (Kelly fraction: {:.2f})",
                m_bettingConfig.kelly_fraction);

    // Initialize game state tracker
    m_gameState = std::make_unique<GameStateTracker>();
    logger.info("Game state tracker initialized");

    // Initialize audio alerts
    m_audioAlerts = std::make_unique<ui::AudioAlertManager>();
    if (!m_audioAlerts->initialize()) {
        logger.warn("Audio alerts initialization failed, continuing without audio");
    }

    m_initialized = true;
    logger.info("Realtime Advisor fully initialized");

    // Announce ready
    logger.info("========================================");
    logger.info("  BLACKJACK ADVISOR READY");
    logger.info("  Audio Signals:");
    logger.info("    Silent = Stand");
    logger.info("    1 beep = Hit");
    logger.info("    2 beeps = Double");
    logger.info("    3 beeps = Split");
    logger.info("    4 beeps = Surrender");
    logger.info("========================================");

    return true;
}

void RealtimeAdvisor::shutdown() {
    if (m_audioAlerts) {
        m_audioAlerts->shutdown();
    }
    m_initialized = false;
}

void RealtimeAdvisor::processFrame(const float* inputTensor) {
    if (!m_initialized) return;

    auto& logger = utils::Logger::getInstance();

    // Run inference
    std::vector<Detection> detections;
    if (!m_visionEngine->infer(inputTensor, detections,
                               m_visionConfig.confidence_threshold,
                               m_visionConfig.nms_threshold)) {
        logger.debug("Inference failed or no detections");
        return;
    }

    if (detections.empty()) {
        return; // No cards detected this frame
    }

    logger.debug("Detected {} cards", detections.size());

    // Update game state with detections
    m_gameState->updateDetectedCards(detections);

    // Update card count
    updateCardCount(detections);

    // Make decision if appropriate
    if (m_gameState->shouldProcessDecision()) {
        makeDecision();
    }

    // Alert on high count
    float trueCount = m_cardCounter->getTrueCount();
    if (trueCount >= 3.0f) {
        // High count - favorable for player
        static auto lastHighCountAlert = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastHighCountAlert).count() > 5) {
            m_audioAlerts->playAlert(ui::AlertType::HighCount);
            lastHighCountAlert = now;
            logger.info("HIGH COUNT ALERT! TC: {:.1f}", trueCount);
        }
    }
}

void RealtimeAdvisor::updateCardCount(const std::vector<Detection>& detections) {
    // Track which cards we've already counted this session
    static std::set<uint8_t> countedCards;

    for (const auto& det : detections) {
        // Only count each unique card once
        if (countedCards.find(det.card_id) == countedCards.end()) {
            Card card;
            card.rank = static_cast<CardRank>((det.card_id % 13) + 1);
            card.suit = static_cast<CardSuit>(det.card_id / 13);
            card.confidence = static_cast<uint8_t>(det.confidence * 100);

            m_cardCounter->addCard(card);
            countedCards.insert(det.card_id);

            auto& logger = utils::Logger::getInstance();
            logger.debug("Counted card: {} | RC: {} | TC: {:.1f}",
                        static_cast<int>(card.rank),
                        m_cardCounter->getRunningCount(),
                        m_cardCounter->getTrueCount());
        }
    }
}

void RealtimeAdvisor::makeDecision() {
    auto& logger = utils::Logger::getInstance();

    const PlayerHand* hand = m_gameState->getCurrentHand();
    const Card* dealerCard = m_gameState->getDealerUpcard();

    if (!hand || !dealerCard) {
        logger.warn("Cannot make decision - missing hand or dealer card");
        return;
    }

    float trueCount = m_cardCounter->getTrueCount();
    int32_t runningCount = m_cardCounter->getRunningCount();

    logger.info("========================================");
    logger.info("DECISION TIME");
    logger.info("Player hand: {} ({})", hand->total, hand->is_soft ? "soft" : "hard");
    logger.info("Dealer upcard: {}", static_cast<int>(dealerCard->rank));
    logger.info("Running Count: {}", runningCount);
    logger.info("True Count: {:.1f}", trueCount);

    // Check for insurance (dealer shows Ace)
    if (dealerCard->rank == CardRank::Ace && trueCount >= 3.0f) {
        logger.info("RECOMMENDATION: Take Insurance (TC >= +3)");
        m_audioAlerts->playAlert(ui::AlertType::Insurance);
        m_lastRecommendedAction = intelligence::Action::Hit; // Insurance is optional
        m_gameState->completeCurrentHand();
        logger.info("========================================");
        return;
    }

    // Get recommended action (with deviations if enabled)
    intelligence::Action action;

    if (m_strategyConfig.deviations_enabled) {
        action = m_basicStrategy->getDeviationAction(
            hand->total,
            dealerCard->rank,
            trueCount);
    } else {
        action = m_basicStrategy->getAction(
            hand->total,
            dealerCard->rank,
            hand->is_soft,
            hand->can_double,
            hand->can_split);
    }

    // Convert action to string for logging
    std::string actionStr;
    switch (action) {
        case intelligence::Action::Hit: actionStr = "HIT"; break;
        case intelligence::Action::Stand: actionStr = "STAND"; break;
        case intelligence::Action::Double: actionStr = "DOUBLE"; break;
        case intelligence::Action::Split: actionStr = "SPLIT"; break;
        case intelligence::Action::Surrender: actionStr = "SURRENDER"; break;
    }

    logger.info("RECOMMENDATION: {}", actionStr);

    // Calculate recommended bet for next hand
    double recommendedBet = m_bettingStrategy->calculateBet(
        trueCount,
        m_bettingStrategy->getBankroll());
    logger.info("Recommended bet for next hand: ${:.2f}", recommendedBet);

    logger.info("========================================");

    // Play audio alert
    ui::AlertType alertType = actionToAlertType(action);
    m_audioAlerts->playAlert(alertType);

    m_lastRecommendedAction = action;
    m_gameState->completeCurrentHand();

    // If split, prepare for multiple hands
    if (action == intelligence::Action::Split) {
        logger.info("Split detected - will track multiple hands");
        // Game state will handle multiple hands in next frame
    }
}

ui::AlertType RealtimeAdvisor::actionToAlertType(intelligence::Action action) {
    switch (action) {
        case intelligence::Action::Hit:
            return ui::AlertType::Hit;
        case intelligence::Action::Stand:
            return ui::AlertType::None;
        case intelligence::Action::Double:
            return ui::AlertType::Double;
        case intelligence::Action::Split:
            return ui::AlertType::Split;
        case intelligence::Action::Surrender:
            return ui::AlertType::Surrender;
        default:
            return ui::AlertType::None;
    }
}

void RealtimeAdvisor::resetCount() {
    auto& logger = utils::Logger::getInstance();
    logger.info("RESETTING COUNT (New Shoe)");

    m_cardCounter->reset();
    m_gameState->resetForNewShoe();
    m_audioAlerts->playAlert(ui::AlertType::CountReset);

    logger.info("Count reset complete");
}

void RealtimeAdvisor::nextHand() {
    auto& logger = utils::Logger::getInstance();
    logger.info("Advancing to next hand");

    if (m_gameState->hasMoreHands()) {
        m_gameState->advanceToNextHand();
    } else {
        m_gameState->startNewHand();
    }
}

void RealtimeAdvisor::forceDecision() {
    auto& logger = utils::Logger::getInstance();
    logger.info("Forcing decision output");

    makeDecision();
}

int32_t RealtimeAdvisor::getRunningCount() const {
    return m_cardCounter->getRunningCount();
}

float RealtimeAdvisor::getTrueCount() const {
    return m_cardCounter->getTrueCount();
}

double RealtimeAdvisor::getRecommendedBet() const {
    return m_bettingStrategy->calculateBet(
        m_cardCounter->getTrueCount(),
        m_bettingStrategy->getBankroll());
}

} // namespace core
