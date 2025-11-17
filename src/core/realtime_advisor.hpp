#pragma once

#include "game_state_tracker.hpp"
#include "../intelligence/counting/card_counter.hpp"
#include "../intelligence/counting/shuffle_detector.hpp"
#include "../intelligence/strategy/basic_strategy.hpp"
#include "../intelligence/strategy/betting_strategy.hpp"
#include "../ui/alerts/audio_alert_manager.hpp"
#include "../vision/inference/tensorrt_engine.hpp"
#include <memory>

namespace core {

// Real-time advisor that processes game state and provides audio guidance
class RealtimeAdvisor {
public:
    RealtimeAdvisor(const VisionConfig& visionConfig,
                    const CountingConfig& countingConfig,
                    const StrategyConfig& strategyConfig,
                    const BettingConfig& bettingConfig);
    ~RealtimeAdvisor();

    // Initialize all subsystems
    bool initialize();
    void shutdown();

    // Main processing loop (called every frame)
    void processFrame(const float* inputTensor);

    // Manual controls (keyboard shortcuts)
    void resetCount();           // User presses key to reset count (new shoe)
    void nextHand();             // User presses key to advance to next hand
    void forceDecision();        // User presses key to force decision output

    // Status
    int32_t getRunningCount() const;
    float getTrueCount() const;
    double getRecommendedBet() const;
    float getCurrentPenetration() const;

private:
    void makeDecision();
    void updateCardCount(const std::vector<Detection>& detections);
    void checkAndHandleShuffleDetection();
    ui::AlertType actionToAlertType(intelligence::Action action);

    // Subsystems
    std::unique_ptr<vision::TensorRTEngine> m_visionEngine;
    std::unique_ptr<intelligence::CardCounter> m_cardCounter;
    std::unique_ptr<intelligence::ShuffleDetector> m_shuffleDetector;
    std::unique_ptr<intelligence::BasicStrategy> m_basicStrategy;
    std::unique_ptr<intelligence::BettingStrategy> m_bettingStrategy;
    std::unique_ptr<GameStateTracker> m_gameState;
    std::unique_ptr<ui::AudioAlertManager> m_audioAlerts;

    // Configuration
    const VisionConfig& m_visionConfig;
    const CountingConfig& m_countingConfig;
    const StrategyConfig& m_strategyConfig;
    const BettingConfig& m_bettingConfig;

    // State
    bool m_initialized{false};
    intelligence::Action m_lastRecommendedAction{intelligence::Action::Hit};
};

} // namespace core
