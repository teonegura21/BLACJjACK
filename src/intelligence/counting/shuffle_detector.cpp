#include "shuffle_detector.hpp"
#include "../../utils/logger.hpp"
#include <algorithm>

namespace intelligence {

// CardInventory implementation
void CardInventory::reset() {
    cards_seen.fill(0);
    total_cards_seen = 0;
}

bool CardInventory::addCard(uint8_t card_id) {
    if (card_id >= 52) return false;

    cards_seen[card_id]++;
    total_cards_seen++;

    return true;
}

bool CardInventory::isImpossible() const {
    // Check if any card appears more times than possible
    // In a 6-deck shoe, each unique card can appear at most 6 times
    // (Ace of Hearts appears once per deck, so 6 times in 6 decks)

    for (uint8_t count : cards_seen) {
        if (count > deck_count) {
            return true; // Impossible: saw same card more than deck count allows
        }
    }

    // Check total cards
    uint32_t max_total = deck_count * 52;
    if (total_cards_seen > max_total) {
        return true; // Impossible: more cards than exist in shoe
    }

    return false;
}

float CardInventory::getPenetration() const {
    uint32_t total_cards_in_shoe = deck_count * 52;
    if (total_cards_in_shoe == 0) return 0.0f;

    return static_cast<float>(total_cards_seen) / total_cards_in_shoe;
}

bool CardInventory::hasReachedPenetrationLimit(float limit) const {
    return getPenetration() >= limit;
}

// ShuffleDetector implementation
ShuffleDetector::ShuffleDetector() {
    m_lastCardDetection = std::chrono::steady_clock::now();
    m_lastResetTime = m_lastCardDetection;
}

ShuffleDetector::~ShuffleDetector() = default;

void ShuffleDetector::initialize(uint32_t deck_count, float penetration_limit) {
    m_deckCount = deck_count;
    m_penetrationLimit = penetration_limit;

    m_inventory.deck_count = deck_count;
    m_inventory.reset();

    auto& logger = utils::Logger::getInstance();
    logger.info("Shuffle detector initialized: {} decks, {:.0f}% penetration limit",
                deck_count, penetration_limit * 100);
}

void ShuffleDetector::reset() {
    auto& logger = utils::Logger::getInstance();
    logger.info("Shuffle detector reset");

    m_inventory.reset();
    m_shuffleDetected = false;
    m_lastIndicator = ShuffleIndicator::None;
    m_previousFrameCardCount = 0;
    m_previousFrameCards.clear();
    m_recentCards.clear();
    m_consecutiveEmptyFrames = 0;
    m_lastResetTime = std::chrono::steady_clock::now();
}

void ShuffleDetector::update(const std::vector<core::Detection>& detections) {
    // Update last card detection time if cards present
    if (!detections.empty()) {
        m_lastCardDetection = std::chrono::steady_clock::now();
        m_consecutiveEmptyFrames = 0;
    } else {
        m_consecutiveEmptyFrames++;
    }

    // Track cards in current frame
    std::vector<uint8_t> currentFrameCards;
    for (const auto& det : detections) {
        currentFrameCards.push_back(det.card_id);

        // Check for duplicate card (same card appearing twice)
        checkDuplicateCard(det.card_id);

        // Add to inventory (only count unique cards once per session)
        static std::set<uint8_t> session_cards;
        if (session_cards.find(det.card_id) == session_cards.end()) {
            m_inventory.addCard(det.card_id);
            session_cards.insert(det.card_id);
        }

        // Add to recent cards list
        m_recentCards.push_back(det.card_id);

        // Keep only last MAX_RECENT_CARDS
        if (m_recentCards.size() > MAX_RECENT_CARDS) {
            m_recentCards.erase(m_recentCards.begin());
        }
    }

    // Run detection checks
    checkCardDepletion();
    checkPenetration();
    checkInactivity();
    checkCardDisappearance();
    checkImpossibleSequence();

    // Store for next frame
    m_previousFrameCardCount = detections.size();
    m_previousFrameCards = currentFrameCards;
}

void ShuffleDetector::checkCardDepletion() {
    // Check if we've seen impossible card combinations
    if (m_inventory.isImpossible()) {
        auto& logger = utils::Logger::getInstance();
        logger.warn("Card depletion detected: Impossible card count");
        triggerShuffleDetection(ShuffleIndicator::CardDepletion);
    }
}

void ShuffleDetector::checkPenetration() {
    // Only check penetration if we've seen enough cards to be sure
    if (m_inventory.total_cards_seen < m_minCardsBeforeReset) {
        return;
    }

    // Check if penetration limit reached
    if (m_inventory.hasReachedPenetrationLimit(m_penetrationLimit)) {
        auto& logger = utils::Logger::getInstance();
        logger.info("Penetration limit reached: {:.1f}% of shoe dealt",
                    m_inventory.getPenetration() * 100);
        triggerShuffleDetection(ShuffleIndicator::PenetrationReached);
    }
}

void ShuffleDetector::checkInactivity() {
    // Only check inactivity if we've seen cards before
    if (m_inventory.total_cards_seen < m_minCardsBeforeReset) {
        return;
    }

    auto timeSinceLastCard = getTimeSinceLastCard();

    if (timeSinceLastCard.count() >= m_inactivityThresholdSec) {
        auto& logger = utils::Logger::getInstance();
        logger.info("Long pause detected: {} seconds since last card",
                    timeSinceLastCard.count());
        triggerShuffleDetection(ShuffleIndicator::LongPause);
    }
}

void ShuffleDetector::checkCardDisappearance() {
    // Detect sudden disappearance of all cards (shuffle in progress)
    // Require many consecutive empty frames to avoid false positives

    if (m_consecutiveEmptyFrames >= EMPTY_FRAMES_THRESHOLD) {
        // Cards were present, now all gone for extended period
        if (m_inventory.total_cards_seen >= m_minCardsBeforeReset) {
            auto& logger = utils::Logger::getInstance();
            logger.info("All cards disappeared - likely shuffle in progress");
            triggerShuffleDetection(ShuffleIndicator::AllCardsGone);
        }

        // Reset counter to prevent repeated triggers
        m_consecutiveEmptyFrames = 0;
    }
}

void ShuffleDetector::checkDuplicateCard(uint8_t card_id) {
    // Check if this exact card has been seen in recent history
    // If the same card appears twice, it's impossible without a shuffle

    // Only check if we've seen enough cards (avoid false positives at start)
    if (m_recentCards.size() < 10) {
        return;
    }

    // Search for this card_id in recent history
    for (size_t i = 0; i < m_recentCards.size() - 1; i++) {  // -1 to exclude the one we just added
        if (m_recentCards[i] == card_id) {
            auto& logger = utils::Logger::getInstance();
            logger.warn("Duplicate card detected: Card ID {} appeared twice (impossible!)", card_id);
            logger.info("This means shuffle occurred but wasn't shown on camera");
            triggerShuffleDetection(ShuffleIndicator::DuplicateCard);
            return;
        }
    }
}

void ShuffleDetector::checkImpossibleSequence() {
    // This is now handled by checkDuplicateCard()
    // Called for each detected card in update()
}

void ShuffleDetector::triggerShuffleDetection(ShuffleIndicator indicator) {
    if (m_shuffleDetected) {
        return; // Already detected, don't trigger again
    }

    auto& logger = utils::Logger::getInstance();

    std::string indicatorStr;
    switch (indicator) {
        case ShuffleIndicator::CardDepletion:
            indicatorStr = "Card Depletion (impossible count)";
            break;
        case ShuffleIndicator::PenetrationReached:
            indicatorStr = "Penetration Limit Reached";
            break;
        case ShuffleIndicator::LongPause:
            indicatorStr = "Long Pause (30+ seconds)";
            break;
        case ShuffleIndicator::AllCardsGone:
            indicatorStr = "All Cards Disappeared";
            break;
        case ShuffleIndicator::DuplicateCard:
            indicatorStr = "Duplicate Card (same card appeared twice!)";
            break;
        case ShuffleIndicator::ImpossibleSequence:
            indicatorStr = "Impossible Card Sequence";
            break;
        case ShuffleIndicator::VisualCue:
            indicatorStr = "Visual Cue Detected";
            break;
        default:
            indicatorStr = "Unknown";
    }

    logger.info("========================================");
    logger.info("SHUFFLE DETECTED: {}", indicatorStr);
    logger.info("Cards seen: {}", m_inventory.total_cards_seen);
    logger.info("Penetration: {:.1f}%", m_inventory.getPenetration() * 100);
    logger.info("AUTO-RESETTING COUNT");
    logger.info("========================================");

    m_shuffleDetected = true;
    m_lastIndicator = indicator;
}

bool ShuffleDetector::isShuffleDetected() const {
    return m_shuffleDetected;
}

void ShuffleDetector::forceReset() {
    auto& logger = utils::Logger::getInstance();
    logger.info("Manual shuffle detection override");
    reset();
}

float ShuffleDetector::getCurrentPenetration() const {
    return m_inventory.getPenetration();
}

bool ShuffleDetector::isPenetrationLimitReached() const {
    return m_inventory.hasReachedPenetrationLimit(m_penetrationLimit);
}

std::chrono::seconds ShuffleDetector::getTimeSinceLastCard() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(
        now - m_lastCardDetection);
}

} // namespace intelligence
