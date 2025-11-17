/**
 * Headless Blackjack Advisor - Audio-Only Mode
 * Real-time card detection with audio guidance
 */

#include "core/realtime_advisor.hpp"
#include "core/config_manager.hpp"
#include "capture/capture_interface.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

// Global flag for graceful shutdown
std::atomic<bool> g_running{true};

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_running = false;
    }
}

// Non-blocking keyboard input
int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

void printHelp() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  BLACKJACK ADVISOR - AUDIO MODE\n";
    std::cout << "========================================\n";
    std::cout << "\n";
    std::cout << "Audio Signals:\n";
    std::cout << "  Silent       = STAND\n";
    std::cout << "  1 beep       = HIT\n";
    std::cout << "  2 beeps      = DOUBLE\n";
    std::cout << "  3 beeps      = SPLIT\n";
    std::cout << "  4 beeps      = SURRENDER\n";
    std::cout << "  5 fast beeps = INSURANCE\n";
    std::cout << "\n";
    std::cout << "Keyboard Controls:\n";
    std::cout << "  R = Reset count (manual override)\n";
    std::cout << "  N = Next hand\n";
    std::cout << "  D = Force decision\n";
    std::cout << "  S = Show status (RC, TC, penetration, bet)\n";
    std::cout << "  H = Show help\n";
    std::cout << "  Q = Quit\n";
    std::cout << "\n";
    std::cout << "The system will automatically:\n";
    std::cout << "  - Detect cards from your screen\n";
    std::cout << "  - Count cards using Hi-Lo\n";
    std::cout << "  - Apply Illustrious 18 deviations\n";
    std::cout << "  - Give you audio signals for actions\n";
    std::cout << "  - AUTO-RESET count when shuffle detected!\n";
    std::cout << "\n";
    std::cout << "Auto-Reset Triggers:\n";
    std::cout << "  - Penetration: 75%+ of shoe dealt\n";
    std::cout << "  - Duplicate card: Same card appears twice!\n";
    std::cout << "  - Inactivity: 30+ seconds no cards\n";
    std::cout << "  - Card depletion: Impossible card count\n";
    std::cout << "  - All cards gone: Shuffle in progress\n";
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "\n";
}

void showStatus(const core::RealtimeAdvisor& advisor) {
    std::cout << "\n";
    std::cout << "--- STATUS ---\n";
    std::cout << "Running Count: " << advisor.getRunningCount() << "\n";
    std::cout << "True Count: " << std::fixed << std::setprecision(1)
              << advisor.getTrueCount() << "\n";
    std::cout << "Penetration: " << std::fixed << std::setprecision(1)
              << advisor.getCurrentPenetration() * 100 << "%\n";
    std::cout << "Recommended Bet: $" << std::fixed << std::setprecision(2)
              << advisor.getRecommendedBet() << "\n";
    std::cout << "--------------\n";
    std::cout << std::flush;
}

int main(int argc, char** argv) {
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize logger
    auto& logger = utils::Logger::getInstance();
    logger.init("logs/blackjack_advisor.log");
    logger.info("Starting Blackjack Advisor (Headless Mode)");

    // Load configuration
    core::ConfigManager configManager;
    if (!configManager.load("config.json")) {
        logger.error("Failed to load configuration");
        return -1;
    }

    auto visionConfig = configManager.getVisionConfig();
    auto countingConfig = configManager.getCountingConfig();
    auto strategyConfig = configManager.getStrategyConfig();
    auto bettingConfig = configManager.getBettingConfig();

    // Create realtime advisor
    core::RealtimeAdvisor advisor(
        visionConfig,
        countingConfig,
        strategyConfig,
        bettingConfig);

    if (!advisor.initialize()) {
        logger.error("Failed to initialize advisor");
        return -1;
    }

    // Print help
    printHelp();

    logger.info("Advisor initialized - Starting main loop");
    std::cout << "Watching for cards... (Press H for help)\n" << std::flush;

    // Main loop
    int frameCount = 0;
    auto startTime = std::chrono::steady_clock::now();

    while (g_running) {
        // Check for keyboard input
        if (kbhit()) {
            char key = getchar();
            key = toupper(key);

            switch (key) {
                case 'R':
                    std::cout << "\n[RESET COUNT]\n" << std::flush;
                    advisor.resetCount();
                    break;

                case 'N':
                    std::cout << "\n[NEXT HAND]\n" << std::flush;
                    advisor.nextHand();
                    break;

                case 'D':
                    std::cout << "\n[FORCE DECISION]\n" << std::flush;
                    advisor.forceDecision();
                    break;

                case 'S':
                    showStatus(advisor);
                    break;

                case 'H':
                    printHelp();
                    break;

                case 'Q':
                    std::cout << "\n[QUIT]\n" << std::flush;
                    g_running = false;
                    break;
            }
        }

        // TODO: Capture frame from screen
        // For now, use dummy input tensor
        // In production, integrate with DXGI capture

        // Simulate frame processing at 30 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(33));

        frameCount++;

        // Show FPS every 5 seconds
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - startTime).count();

        if (elapsed >= 5) {
            float fps = frameCount / (float)elapsed;
            logger.info("Processing at {:.1f} FPS", fps);
            frameCount = 0;
            startTime = now;
        }
    }

    // Cleanup
    logger.info("Shutting down advisor");
    advisor.shutdown();

    std::cout << "\nGoodbye!\n";
    return 0;
}
