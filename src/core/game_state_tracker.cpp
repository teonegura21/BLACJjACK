#include "game_state_tracker.hpp"
#include "../utils/logger.hpp"
#include <algorithm>

namespace core {

GameStateTracker::GameStateTracker() {
    m_lastUpdate = std::chrono::steady_clock::now();
    m_lastDecision = m_lastUpdate;
    startNewHand();
}

GameStateTracker::~GameStateTracker() = default;

void GameStateTracker::startNewHand() {
    auto& logger = utils::Logger::getInstance();
    logger.info("Starting new hand");

    m_playerHands.clear();
    m_playerHands.push_back(PlayerHand{});
    m_currentHandIndex = 0;

    m_dealerUpcard.reset();
    m_detectedCardIds.clear();
    m_cardStability.clear();

    m_phase = GamePhase::WaitingForCards;
    m_decisionMade = false;
}

void GameStateTracker::updateDetectedCards(const std::vector<Detection>& detections) {
    m_lastUpdate = std::chrono::steady_clock::now();

    // Update card stability tracking
    std::vector<uint8_t> currentFrameCards;
    for (const auto& det : detections) {
        currentFrameCards.push_back(det.card_id);
    }

    // Update stability counters
    for (auto& [card_id, frames] : m_cardStability) {
        if (std::find(currentFrameCards.begin(), currentFrameCards.end(), card_id)
            != currentFrameCards.end()) {
            frames++;
        } else {
            frames = 0; // Reset if not detected this frame
        }
    }

    // Add new cards to stability tracking
    for (uint8_t card_id : currentFrameCards) {
        bool found = false;
        for (auto& [id, frames] : m_cardStability) {
            if (id == card_id) {
                found = true;
                break;
            }
        }
        if (!found) {
            m_cardStability.push_back({card_id, 1});
        }
    }

    // Only process stable cards
    std::vector<uint8_t> stableCards;
    for (const auto& [card_id, frames] : m_cardStability) {
        if (frames >= STABILITY_FRAMES) {
            stableCards.push_back(card_id);
        }
    }

    // Update game state based on stable detections
    if (m_phase == GamePhase::WaitingForCards) {
        // Expecting: 2 player cards + 1 dealer upcard
        if (stableCards.size() >= 3) {
            // Assign cards (simple heuristic - first 2 to player, 3rd to dealer)
            // In production, use position/geometry to determine ownership
            auto& hand = m_playerHands[0];
            hand.cards.clear();

            for (size_t i = 0; i < 2 && i < stableCards.size(); i++) {
                Card card;
                card.rank = static_cast<CardRank>((stableCards[i] % 13) + 1);
                card.suit = static_cast<CardSuit>(stableCards[i] / 13);
                card.confidence = 100;
                hand.cards.push_back(card);
            }

            // Dealer upcard
            if (stableCards.size() >= 3) {
                Card dealerCard;
                dealerCard.rank = static_cast<CardRank>((stableCards[2] % 13) + 1);
                dealerCard.suit = static_cast<CardSuit>(stableCards[2] / 13);
                dealerCard.confidence = 100;
                m_dealerUpcard = dealerCard;
            }

            analyzeHands();
            m_phase = GamePhase::PlayerTurn;
            m_decisionMade = false;

            auto& logger = utils::Logger::getInstance();
            logger.info("Initial cards detected - Player turn");
        }
    }
}

void GameStateTracker::analyzeHands() {
    for (auto& hand : m_playerHands) {
        calculateHandTotal(hand);
    }
    detectPairs();
}

void GameStateTracker::calculateHandTotal(PlayerHand& hand) {
    if (hand.cards.empty()) return;

    int total = 0;
    int aces = 0;

    for (const auto& card : hand.cards) {
        int value = card.getValue();
        if (card.rank == CardRank::Ace) {
            aces++;
        }
        total += value;
    }

    // Adjust for soft aces
    hand.is_soft = false;
    while (total > 21 && aces > 0) {
        total -= 10; // Convert ace from 11 to 1
        aces--;
    }

    if (aces > 0) {
        hand.is_soft = true;
    }

    hand.total = total;
    hand.is_busted = (total > 21);
    hand.is_blackjack = (hand.cards.size() == 2 && total == 21);

    // Can only double on first two cards
    hand.can_double = (hand.cards.size() == 2);
}

void GameStateTracker::detectPairs() {
    auto& hand = m_playerHands[m_currentHandIndex];

    if (hand.cards.size() == 2) {
        hand.is_pair = (hand.cards[0].rank == hand.cards[1].rank);
        hand.can_split = hand.is_pair;
    } else {
        hand.is_pair = false;
        hand.can_split = false;
    }
}

const PlayerHand* GameStateTracker::getCurrentHand() const {
    if (m_currentHandIndex >= 0 && m_currentHandIndex < (int)m_playerHands.size()) {
        return &m_playerHands[m_currentHandIndex];
    }
    return nullptr;
}

const Card* GameStateTracker::getDealerUpcard() const {
    if (m_dealerUpcard.has_value()) {
        return &m_dealerUpcard.value();
    }
    return nullptr;
}

bool GameStateTracker::areInitialCardsDetected() const {
    if (m_playerHands.empty()) return false;
    const auto& hand = m_playerHands[0];
    return (hand.cards.size() >= 2 && m_dealerUpcard.has_value());
}

bool GameStateTracker::isDealerUpcardDetected() const {
    return m_dealerUpcard.has_value();
}

bool GameStateTracker::shouldProcessDecision() const {
    // Only process decisions during player's turn
    if (m_phase != GamePhase::PlayerTurn) return false;

    // Must have cards detected
    if (!areInitialCardsDetected()) return false;

    // Don't make duplicate decisions
    if (m_decisionMade) return false;

    // Require some time between decisions (debounce)
    auto timeSinceLastDecision = std::chrono::steady_clock::now() - m_lastDecision;
    if (timeSinceLastDecision < std::chrono::milliseconds(1000)) {
        return false;
    }

    return true;
}

void GameStateTracker::completeCurrentHand() {
    if (m_currentHandIndex < (int)m_playerHands.size()) {
        m_playerHands[m_currentHandIndex].is_completed = true;
    }
    m_decisionMade = true;
    m_lastDecision = std::chrono::steady_clock::now();
}

void GameStateTracker::advanceToNextHand() {
    if (hasMoreHands()) {
        m_currentHandIndex++;
        m_decisionMade = false;

        auto& logger = utils::Logger::getInstance();
        logger.info("Advancing to split hand {}", m_currentHandIndex + 1);
    } else {
        m_phase = GamePhase::HandComplete;
    }
}

void GameStateTracker::markHandComplete() {
    auto& logger = utils::Logger::getInstance();
    logger.info("Hand marked complete by user");

    m_phase = GamePhase::HandComplete;
}

void GameStateTracker::resetForNewShoe() {
    auto& logger = utils::Logger::getInstance();
    logger.info("Resetting for new shoe");

    startNewHand();
    m_phase = GamePhase::NewShoe;
}

void GameStateTracker::manualCountReset() {
    auto& logger = utils::Logger::getInstance();
    logger.info("Manual count reset requested");

    resetForNewShoe();
}

std::chrono::milliseconds GameStateTracker::getTimeSinceLastUpdate() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastUpdate);
}

} // namespace core
