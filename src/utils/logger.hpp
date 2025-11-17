#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <fstream>

namespace utils {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    static Logger& getInstance();
    
    void init(const std::string& logFilePath);
    void setLevel(LogLevel level);
    
    template<typename... Args>
    void debug(const std::string& format, Args&&... args) {
        log(LogLevel::DEBUG, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void info(const std::string& format, Args&&... args) {
        log(LogLevel::INFO, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warning(const std::string& format, Args&&... args) {
        log(LogLevel::WARNING, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void error(const std::string& format, Args&&... args) {
        log(LogLevel::ERROR, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void critical(const std::string& format, Args&&... args) {
        log(LogLevel::CRITICAL, format, std::forward<Args>(args)...);
    }

private:
    Logger() = default;
    ~Logger();
    
    template<typename... Args>
    void log(LogLevel level, const std::string& format, Args&&... args);
    
    std::string levelToString(LogLevel level);
    std::string getCurrentTimestamp();
    
    std::ofstream m_logFile;
    std::mutex m_mutex;
    LogLevel m_minLevel{LogLevel::INFO};
};

// Template implementation
template<typename... Args>
void Logger::log(LogLevel level, const std::string& format, Args&&... args) {
    if (level < m_minLevel) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Simple formatting (in production, use fmt library)
    std::string message = format;
    
    std::string logEntry = "[" + getCurrentTimestamp() + "] [" + 
                          levelToString(level) + "] " + message + "\n";
    
    if (m_logFile.is_open()) {
        m_logFile << logEntry;
        m_logFile.flush();
    }
}

} // namespace utils
