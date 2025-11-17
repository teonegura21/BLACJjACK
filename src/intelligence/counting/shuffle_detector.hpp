#pragma once

#include "../../core/types.hpp"
#include <array>
#include <chrono>
#include <vector>

namespace intelligence {

// Shuffle detection methods
enum class ShuffleIndicator {
    None,
    CardDepletion,      // Impossible card count (more than deck allows)
    PenetrationReached, // 75%+ of shoe dealt
    LongPause,          // 30+ seconds no cards detected
    AllCardsGone,       // All cards disappeared from screen
    ImpossibleSequence, // Card sequence that shouldn't happen
    VisualCue          // Detected "shuffle" text or cut card
};

// Tracks individual card counts to detect impossible situations
struct CardInventory {
    std::array<uint8_t, 52> cards_seen{};  // Count for each of 52 cards
    uint32_t total_cards_seen{0};
    uint32_t deck_count{6};

    void reset();
    bool addCard(uint8_t card_id);
    bool isImpossible() const;
    float getPenetration() const;
    bool hasReachedPenetrationLimit(float limit = 0.75f) const;
};

class ShuffleDetector {
public:
    ShuffleDetector();
    ~ShuffleDetector();

    void initialize(uint32_t deck_count, float penetration_limit = 0.75f);
    void reset();

    // Update with each frame's detections
    void update(const std::vector<core::Detection>& detections);

    // Check if shuffle detected
    bool isShuffleDetected() const;
    ShuffleIndicator getLastIndicator() const { return m_lastIndicator; }

    // Manual override
    void forceReset();

    // Status
    float getCurrentPenetration() const;
    bool isPenetrationLimitReached() const;
    std::chrono::seconds getTimeSinceLastCard() const;

    // Configuration
    void setPenetrationLimit(float limit) { m_penetrationLimit = limit; }
    void setInactivityThreshold(int seconds) { m_inactivityThresholdSec = seconds; }
    void setMinCardsForReset(int min) { m_minCardsBeforeReset = min; }

private:
    void checkCardDepletion();
    void checkPenetration();
    void checkInactivity();
    void checkCardDisappearance();
    void checkImpossibleSequence();

    void triggerShuffleDetection(ShuffleIndicator indicator);

    // Card tracking
    CardInventory m_inventory;

    // Timing
    std::chrono::steady_clock::time_point m_lastCardDetection;
    std::chrono::steady_clock::time_point m_lastResetTime;

    // Detection state
    bool m_shuffleDetected{false};
    ShuffleIndicator m_lastIndicator{ShuffleIndicator::None};

    // Previous frame state
    uint32_t m_previousFrameCardCount{0};
    std::vector<uint8_t> m_previousFrameCards;

    // Configuration
    uint32_t m_deckCount{6};
    float m_penetrationLimit{0.75f};
    int m_inactivityThresholdSec{30};
    int m_minCardsBeforeReset{26}; // At least half deck before allowing auto-reset

    // Counters
    uint32_t m_consecutiveEmptyFrames{0};
    static constexpr uint32_t EMPTY_FRAMES_THRESHOLD = 60; // 2 seconds at 30 FPS
};

} // namespace intelligence
