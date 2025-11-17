#pragma once

#include "../core/types.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <json-c/json.h>

namespace utils {

// Single hand record
struct HandRecord {
    uint64_t hand_number;
    std::vector<std::string> player_cards;
    std::string dealer_upcard;
    int32_t running_count;
    float true_count;
    std::string recommended_action;
    std::string actual_action;
    std::string result;  // "Win", "Loss", "Push"
    double bet_amount;
    double payout;
    uint64_t timestamp_ms;
};

// Session summary statistics
struct SessionSummary {
    uint64_t total_hands{0};
    uint32_t wins{0};
    uint32_t losses{0};
    uint32_t pushes{0};
    double total_wagered{0.0};
    double total_profit{0.0};
    double max_win{0.0};
    double max_loss{0.0};
    double max_drawdown{0.0};
    double final_bankroll{0.0};
    float avg_true_count{0.0f};
    float max_true_count{0.0f};
    float min_true_count{0.0f};
    uint32_t insurance_taken{0};
    uint32_t doubles{0};
    uint32_t splits{0};
    uint32_t surrenders{0};
    uint64_t duration_seconds{0};
    float strategy_adherence{0.0f}; // % of decisions following recommendation
};

class SessionRecorder {
public:
    SessionRecorder();
    ~SessionRecorder();

    // Session management
    void startSession(const std::string& session_id = "");
    void endSession();
    bool isRecording() const { return m_recording; }

    // Recording
    void recordHand(const HandRecord& hand);
    void updateSummary();

    // Export
    bool exportToJSON(const std::string& filepath);
    bool exportToCSV(const std::string& filepath);

    // Getters
    const SessionSummary& getSummary() const { return m_summary; }
    const std::vector<HandRecord>& getHands() const { return m_hands; }
    std::string getSessionId() const { return m_sessionId; }

    // Analytics
    float calculateWinRate() const;
    float calculateROI() const;
    std::vector<double> getBankrollHistory() const;

private:
    std::string generateSessionId();
    json_object* handToJSON(const HandRecord& hand);
    json_object* summaryToJSON();

    bool m_recording{false};
    std::string m_sessionId;
    std::vector<HandRecord> m_hands;
    SessionSummary m_summary;

    std::chrono::time_point<std::chrono::system_clock> m_sessionStart;
    double m_startingBankroll{10000.0};
};

} // namespace utils
