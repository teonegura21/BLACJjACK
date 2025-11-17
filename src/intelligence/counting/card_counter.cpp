#include "card_counter.hpp"
#include "../../utils/logger.hpp"
#include <cmath>

namespace intelligence {

CardCounter::CardCounter() {
}

CardCounter::~CardCounter() {
}

void CardCounter::initialize(uint32_t deckCount) {
    m_deckCount = deckCount;
    reset();
}

void CardCounter::reset() {
    m_runningCount = 0;
    m_trueCount = 0.0f;
    m_confidence = 1.0f;
    m_cardsPlayed = 0;
    m_cardsSeen.fill(0);
}

void CardCounter::addCard(const core::Card& card) {
    // Update running count using Hi-Lo system
    int cardValue = getHiLoValue(card.rank);
    m_runningCount += cardValue;
    m_cardsPlayed++;
    
    // Update card tracking
    uint8_t cardIndex = static_cast<uint8_t>(card.rank) + 
                       static_cast<uint8_t>(card.suit) * 13;
    if (cardIndex < 52) {
        m_cardsSeen[cardIndex]++;
    }
    
    updateTrueCount();
    updateConfidence();
}

float CardCounter::getTrueCount() const {
    return m_trueCount;
}

int CardCounter::getHiLoValue(core::CardRank rank) const {
    // Hi-Lo counting system
    int rankValue = static_cast<int>(rank);
    
    if (rankValue >= 2 && rankValue <= 6) {
        return 1;  // Low cards: +1
    } else if (rankValue >= 10 || rank == core::CardRank::Ace) {
        return -1;  // High cards and Aces: -1
    }
    return 0;  // Neutral cards (7, 8, 9)
}

void CardCounter::updateTrueCount() {
    uint32_t cardsRemaining = getCardsRemaining();
    if (cardsRemaining == 0) {
        m_trueCount = 0.0f;
        return;
    }
    
    float decksRemaining = cardsRemaining / 52.0f;
    m_trueCount = static_cast<float>(m_runningCount) / decksRemaining;
}

void CardCounter::updateConfidence() {
    // Confidence decreases with more cards played
    float penetration = getPenetration();
    m_confidence = 1.0f - (penetration * 0.5f);  // Simple linear model
}

uint32_t CardCounter::getCardsRemaining() const {
    uint32_t totalCards = m_deckCount * 52;
    return totalCards > m_cardsPlayed ? totalCards - m_cardsPlayed : 0;
}

float CardCounter::getPenetration() const {
    uint32_t totalCards = m_deckCount * 52;
    return static_cast<float>(m_cardsPlayed) / totalCards;
}

} // namespace intelligence
