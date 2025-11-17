/**
 * Real-Time Blackjack Card Counting System
 * Main Application Entry Point
 */

#include "core/application.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <memory>

int main(int argc, char** argv) {
    try {
        // Initialize logger
        auto& logger = utils::Logger::getInstance();
        logger.init("logs/blackjack_ai.log");
        logger.info("Starting Blackjack AI Vision System...");

        // Create and run application
        auto app = std::make_unique<core::Application>();
        
        if (!app->initialize()) {
            logger.error("Failed to initialize application");
            return -1;
        }

        logger.info("Application initialized successfully");
        
        // Run main loop
        int exitCode = app->run();
        
        // Cleanup
        app->shutdown();
        logger.info("Application shutdown complete");
        
        return exitCode;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}
