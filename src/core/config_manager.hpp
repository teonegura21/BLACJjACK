#pragma once

#include <string>
#include <memory>
#include "types.hpp"

namespace core {

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    bool load(const std::string& configPath);
    bool save(const std::string& configPath);
    bool reload();

    // System configuration
    const SystemConfig& getSystemConfig() const { return m_systemConfig; }
    
    // Capture configuration
    const CaptureConfig& getCaptureConfig() const { return m_captureConfig; }
    
    // Vision configuration
    const VisionConfig& getVisionConfig() const { return m_visionConfig; }
    
    // Counting configuration
    const CountingConfig& getCountingConfig() const { return m_countingConfig; }
    
    // Strategy configuration
    const StrategyConfig& getStrategyConfig() const { return m_strategyConfig; }
    
    // UI configuration
    const UIConfig& getUIConfig() const { return m_uiConfig; }

private:
    SystemConfig m_systemConfig;
    CaptureConfig m_captureConfig;
    VisionConfig m_visionConfig;
    CountingConfig m_countingConfig;
    StrategyConfig m_strategyConfig;
    UIConfig m_uiConfig;
    
    std::string m_configPath;
};

} // namespace core
