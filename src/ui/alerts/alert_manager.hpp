#pragma once

#include <string>
#include <queue>
#include <memory>

namespace ui {

enum class AlertType {
    CountChanged,
    ActionRequired,
    HighCount,
    LowCount,
    ErrorDetected
};

struct Alert {
    AlertType type;
    std::string message;
    uint64_t timestamp;
    uint32_t priority;
};

class AlertManager {
public:
    AlertManager();
    ~AlertManager();

    bool initialize();
    void shutdown();
    
    void triggerAlert(AlertType type, const std::string& message);
    void processAlerts();
    
    void enableAudio(bool enable);
    void enableVisual(bool enable);
    void enableHaptic(bool enable);

private:
    void playAudioAlert(AlertType type);
    void showVisualAlert(const Alert& alert);
    void triggerHapticFeedback(AlertType type);
    
    std::queue<Alert> m_alertQueue;
    
    bool m_audioEnabled{true};
    bool m_visualEnabled{true};
    bool m_hapticEnabled{false};
};

} // namespace ui
