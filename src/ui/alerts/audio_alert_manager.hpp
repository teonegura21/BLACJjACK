#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <chrono>

namespace ui {

// Audio alert types with beep patterns
enum class AlertType {
    None,           // 0 beeps - Stand (silent)
    Hit,            // 1 beep
    Double,         // 2 beeps
    Split,          // 3 beeps
    Surrender,      // 4 beeps
    Insurance,      // 5 fast beeps
    CountReset,     // Long beep
    NewShoe,        // 2 long beeps
    HighCount       // Ascending tone (TC >= +3)
};

class AudioAlertManager {
public:
    AudioAlertManager();
    ~AudioAlertManager();

    // Initialize audio system
    bool initialize();
    void shutdown();

    // Play alerts
    void playAlert(AlertType type);
    void playBeeps(int count, int duration_ms = 200, int pause_ms = 150);
    void playTone(int frequency_hz, int duration_ms);

    // Volume control
    void setVolume(float volume); // 0.0 to 1.0
    float getVolume() const { return m_volume; }

    // Enable/disable
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

    // Announce actions verbally (optional)
    void announceAction(const std::string& action);

private:
    void generateBeep(int frequency_hz, int duration_ms);
    void playSound(const float* samples, int num_samples);

    std::atomic<bool> m_enabled{true};
    std::atomic<float> m_volume{0.7f};
    bool m_initialized{false};

    // Audio device handle (platform-specific)
    void* m_audioDevice{nullptr};
};

} // namespace ui
