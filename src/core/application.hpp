#pragma once

#include <memory>
#include <atomic>
#include "config_manager.hpp"

namespace core {

class Application {
public:
    Application();
    ~Application();

    bool initialize();
    int run();
    void shutdown();
    
    void requestExit() { m_running = false; }
    bool isRunning() const { return m_running; }

private:
    void initializeHardware();
    void initializeThreadPool();
    void startPipeline();
    void stopPipeline();

    std::unique_ptr<ConfigManager> m_configManager;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_initialized{false};
};

} // namespace core
