#pragma once

#include "types.hpp"
#include <vector>
#include <optional>
#include <chrono>

namespace core {

// Represents a single player hand (can have multiple after split)
struct PlayerHand {
    std::vector<Card> cards;
    uint32_t total{0};
    bool is_soft{false};
    bool is_pair{false};
    bool can_double{true};
    bool can_split{false};
    bool is_blackjack{false};
    bool is_busted{false};
    bool is_completed{false};
    int hand_index{0}; // 0 = first hand, 1+ = split hands
};

// Current game state
enum class GamePhase {
    WaitingForCards,    // Waiting for initial cards to be dealt
    PlayerTurn,         // Player's turn to act
    DealerTurn,         // Dealer's turn
    HandComplete,       // Hand finished, waiting for next
    NewShoe             // New shoe detected, reset count
};

class GameStateTracker {
public:
    GameStateTracker();
    ~GameStateTracker();

    // Update with detected cards
    void updateDetectedCards(const std::vector<Detection>& detections);

    // Game flow control
    void startNewHand();
    void completeCurrentHand();
    void advanceToNextHand();
    void resetForNewShoe();

    // Manual controls
    void manualCountReset();
    void markHandComplete();

    // Current state getters
    GamePhase getCurrentPhase() const { return m_phase; }
    const std::vector<PlayerHand>& getPlayerHands() const { return m_playerHands; }
    int getCurrentHandIndex() const { return m_currentHandIndex; }
    const PlayerHand* getCurrentHand() const;
    const Card* getDealerUpcard() const;

    // Context awareness
    bool isSplitScenario() const { return m_playerHands.size() > 1; }
    int getNumHands() const { return m_playerHands.size(); }
    bool hasMoreHands() const { return m_currentHandIndex < (int)m_playerHands.size() - 1; }

    // Detection state
    bool areInitialCardsDetected() const;
    bool isDealerUpcardDetected() const;

    // Timing and validation
    bool shouldProcessDecision() const;
    std::chrono::milliseconds getTimeSinceLastUpdate() const;

private:
    void analyzeHands();
    void calculateHandTotal(PlayerHand& hand);
    void detectPairs();
    void updatePhase();
    Card detectionToCard(const Detection& det);
    bool isCardStable(uint8_t card_id) const;

    GamePhase m_phase{GamePhase::WaitingForCards};

    // Player hands (1 initially, multiple after split)
    std::vector<PlayerHand> m_playerHands;
    int m_currentHandIndex{0};

    // Dealer
    std::optional<Card> m_dealerUpcard;

    // Card detection tracking
    std::vector<uint8_t> m_detectedCardIds;
    std::chrono::steady_clock::time_point m_lastUpdate;
    std::chrono::steady_clock::time_point m_lastDecision;

    // Temporal stability (require card detected for N frames)
    static constexpr int STABILITY_FRAMES = 3;
    std::vector<std::pair<uint8_t, int>> m_cardStability;

    // Prevent duplicate decisions
    bool m_decisionMade{false};
};

} // namespace core
