#include "audio_alert_manager.hpp"
#include "../../utils/logger.hpp"
#include <cmath>
#include <vector>
#include <thread>
#include <iostream>

// Simple beep generation using system commands
// For production, use proper audio library (PortAudio, ALSA, etc.)

namespace ui {

AudioAlertManager::AudioAlertManager() = default;

AudioAlertManager::~AudioAlertManager() {
    shutdown();
}

bool AudioAlertManager::initialize() {
    auto& logger = utils::Logger::getInstance();
    logger.info("Initializing audio alert system");

    // Simple initialization - using system beep for now
    // In production, initialize proper audio library here
    m_initialized = true;

    logger.info("Audio alert system initialized (using system beep)");
    return true;
}

void AudioAlertManager::shutdown() {
    m_enabled = false;
    m_initialized = false;
}

void AudioAlertManager::playAlert(AlertType type) {
    if (!m_enabled || !m_initialized) return;

    auto& logger = utils::Logger::getInstance();

    switch (type) {
        case AlertType::None:
            // Silent - Stand
            logger.info("[AUDIO] Stand (silent)");
            break;

        case AlertType::Hit:
            logger.info("[AUDIO] Hit (1 beep)");
            playBeeps(1);
            break;

        case AlertType::Double:
            logger.info("[AUDIO] Double (2 beeps)");
            playBeeps(2);
            break;

        case AlertType::Split:
            logger.info("[AUDIO] Split (3 beeps)");
            playBeeps(3);
            break;

        case AlertType::Surrender:
            logger.info("[AUDIO] Surrender (4 beeps)");
            playBeeps(4);
            break;

        case AlertType::Insurance:
            logger.info("[AUDIO] Insurance (5 fast beeps)");
            playBeeps(5, 100, 80);
            break;

        case AlertType::CountReset:
            logger.info("[AUDIO] Count Reset (long beep)");
            playBeeps(1, 800, 0);
            break;

        case AlertType::NewShoe:
            logger.info("[AUDIO] New Shoe (2 long beeps)");
            playBeeps(2, 600, 400);
            break;

        case AlertType::HighCount:
            logger.info("[AUDIO] High Count Alert (ascending tone)");
            playTone(800, 200);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            playTone(1000, 200);
            break;
    }
}

void AudioAlertManager::playBeeps(int count, int duration_ms, int pause_ms) {
    if (!m_enabled || !m_initialized) return;

    for (int i = 0; i < count; i++) {
        // Generate beep at 800 Hz
        playTone(800, duration_ms);

        // Pause between beeps (except after last beep)
        if (i < count - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(pause_ms));
        }
    }
}

void AudioAlertManager::playTone(int frequency_hz, int duration_ms) {
    if (!m_enabled || !m_initialized) return;

    // Simple system beep using ANSI escape sequence
    // This works on most Linux terminals
    std::cout << "\a" << std::flush;

    // For more sophisticated audio, use audio library
    // Example with PortAudio or ALSA would go here

    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
}

void AudioAlertManager::setVolume(float volume) {
    m_volume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioAlertManager::announceAction(const std::string& action) {
    if (!m_enabled) return;

    auto& logger = utils::Logger::getInstance();
    logger.info("[VOICE] {}", action);

    // Optional: Use text-to-speech library (espeak, festival, pyttsx3, etc.)
    // Example: system(("espeak \"" + action + "\"").c_str());
}

} // namespace ui
