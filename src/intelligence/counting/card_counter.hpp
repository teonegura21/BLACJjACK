#pragma once

#include "../../core/types.hpp"
#include <array>

namespace intelligence {

class CardCounter {
public:
    CardCounter();
    ~CardCounter();

    void initialize(uint32_t deckCount);
    void reset();
    
    // Update count with new card
    void addCard(const core::Card& card);
    
    // Get current counts
    int32_t getRunningCount() const { return m_runningCount; }
    float getTrueCount() const;
    float getConfidence() const { return m_confidence; }
    
    // Deck penetration
    void setDeckCount(uint32_t count) { m_deckCount = count; }
    uint32_t getCardsRemaining() const;
    float getPenetration() const;

private:
    int getHiLoValue(core::CardRank rank) const;
    void updateTrueCount();
    void updateConfidence();
    
    int32_t m_runningCount{0};
    float m_trueCount{0.0f};
    float m_confidence{1.0f};
    
    uint32_t m_deckCount{6};
    uint32_t m_cardsPlayed{0};
    
    // Card tracking
    std::array<uint8_t, 52> m_cardsSeen{};
};

} // namespace intelligence
