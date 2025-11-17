#include "session_recorder.hpp"
#include "logger.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

namespace utils {

SessionRecorder::SessionRecorder() = default;
SessionRecorder::~SessionRecorder() {
    if (m_recording) {
        endSession();
    }
}

std::string SessionRecorder::generateSessionId() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S");
    ss << "_" << std::setfill('0') << std::setw(3) << now_ms.count();

    return ss.str();
}

void SessionRecorder::startSession(const std::string& session_id) {
    auto& logger = Logger::getInstance();

    if (m_recording) {
        logger.warn("Session already recording, ending previous session");
        endSession();
    }

    m_sessionId = session_id.empty() ? generateSessionId() : session_id;
    m_sessionStart = std::chrono::system_clock::now();
    m_hands.clear();
    m_summary = SessionSummary();
    m_recording = true;

    logger.info("Started recording session: {}", m_sessionId);
}

void SessionRecorder::endSession() {
    if (!m_recording) return;

    auto& logger = Logger::getInstance();

    // Calculate final statistics
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        now - m_sessionStart);
    m_summary.duration_seconds = duration.count();

    updateSummary();

    // Auto-export to JSON
    std::string filepath = "data/sessions/" + m_sessionId + ".json";
    exportToJSON(filepath);

    m_recording = false;

    logger.info("Session {} ended. Duration: {}s, Hands: {}, Profit: ${:.2f}",
                m_sessionId,
                m_summary.duration_seconds,
                m_summary.total_hands,
                m_summary.total_profit);
}

void SessionRecorder::recordHand(const HandRecord& hand) {
    if (!m_recording) return;

    m_hands.push_back(hand);
    updateSummary();
}

void SessionRecorder::updateSummary() {
    m_summary.total_hands = m_hands.size();

    if (m_hands.empty()) return;

    // Reset counters
    m_summary.wins = 0;
    m_summary.losses = 0;
    m_summary.pushes = 0;
    m_summary.total_wagered = 0.0;
    m_summary.total_profit = 0.0;
    m_summary.max_win = 0.0;
    m_summary.max_loss = 0.0;
    m_summary.insurance_taken = 0;
    m_summary.doubles = 0;
    m_summary.splits = 0;
    m_summary.surrenders = 0;

    float total_true_count = 0.0f;
    m_summary.max_true_count = -100.0f;
    m_summary.min_true_count = 100.0f;

    uint32_t strategy_matches = 0;
    double running_bankroll = m_startingBankroll;
    double peak_bankroll = m_startingBankroll;
    double max_dd = 0.0;

    // Process each hand
    for (const auto& hand : m_hands) {
        // Win/Loss/Push
        if (hand.result == "Win") {
            m_summary.wins++;
            m_summary.total_profit += hand.payout;
            if (hand.payout > m_summary.max_win) {
                m_summary.max_win = hand.payout;
            }
        } else if (hand.result == "Loss") {
            m_summary.losses++;
            m_summary.total_profit -= hand.bet_amount;
            double loss = -hand.bet_amount;
            if (loss < m_summary.max_loss) {
                m_summary.max_loss = loss;
            }
        } else if (hand.result == "Push") {
            m_summary.pushes++;
        }

        // Wagered
        m_summary.total_wagered += hand.bet_amount;

        // Count statistics
        total_true_count += hand.true_count;
        if (hand.true_count > m_summary.max_true_count) {
            m_summary.max_true_count = hand.true_count;
        }
        if (hand.true_count < m_summary.min_true_count) {
            m_summary.min_true_count = hand.true_count;
        }

        // Action tracking
        if (hand.actual_action.find("Insurance") != std::string::npos) {
            m_summary.insurance_taken++;
        }
        if (hand.actual_action == "Double") {
            m_summary.doubles++;
        }
        if (hand.actual_action == "Split") {
            m_summary.splits++;
        }
        if (hand.actual_action == "Surrender") {
            m_summary.surrenders++;
        }

        // Strategy adherence
        if (hand.recommended_action == hand.actual_action) {
            strategy_matches++;
        }

        // Bankroll tracking for drawdown
        if (hand.result == "Win") {
            running_bankroll += hand.payout;
        } else if (hand.result == "Loss") {
            running_bankroll -= hand.bet_amount;
        }

        if (running_bankroll > peak_bankroll) {
            peak_bankroll = running_bankroll;
        }

        double current_dd = peak_bankroll - running_bankroll;
        if (current_dd > max_dd) {
            max_dd = current_dd;
        }
    }

    // Averages
    m_summary.avg_true_count = total_true_count / m_summary.total_hands;
    m_summary.strategy_adherence =
        (static_cast<float>(strategy_matches) / m_summary.total_hands) * 100.0f;
    m_summary.final_bankroll = running_bankroll;
    m_summary.max_drawdown = max_dd;
}

json_object* SessionRecorder::handToJSON(const HandRecord& hand) {
    json_object* jhand = json_object_new_object();

    json_object_object_add(jhand, "hand_number",
                          json_object_new_int64(hand.hand_number));

    // Player cards array
    json_object* jplayer_cards = json_object_new_array();
    for (const auto& card : hand.player_cards) {
        json_object_array_add(jplayer_cards, json_object_new_string(card.c_str()));
    }
    json_object_object_add(jhand, "player_cards", jplayer_cards);

    json_object_object_add(jhand, "dealer_upcard",
                          json_object_new_string(hand.dealer_upcard.c_str()));
    json_object_object_add(jhand, "running_count",
                          json_object_new_int(hand.running_count));
    json_object_object_add(jhand, "true_count",
                          json_object_new_double(hand.true_count));
    json_object_object_add(jhand, "recommended_action",
                          json_object_new_string(hand.recommended_action.c_str()));
    json_object_object_add(jhand, "actual_action",
                          json_object_new_string(hand.actual_action.c_str()));
    json_object_object_add(jhand, "result",
                          json_object_new_string(hand.result.c_str()));
    json_object_object_add(jhand, "bet_amount",
                          json_object_new_double(hand.bet_amount));
    json_object_object_add(jhand, "payout",
                          json_object_new_double(hand.payout));
    json_object_object_add(jhand, "timestamp_ms",
                          json_object_new_int64(hand.timestamp_ms));

    return jhand;
}

json_object* SessionRecorder::summaryToJSON() {
    json_object* jsummary = json_object_new_object();

    json_object_object_add(jsummary, "total_hands",
                          json_object_new_int64(m_summary.total_hands));
    json_object_object_add(jsummary, "wins",
                          json_object_new_int(m_summary.wins));
    json_object_object_add(jsummary, "losses",
                          json_object_new_int(m_summary.losses));
    json_object_object_add(jsummary, "pushes",
                          json_object_new_int(m_summary.pushes));
    json_object_object_add(jsummary, "win_rate",
                          json_object_new_double(calculateWinRate()));
    json_object_object_add(jsummary, "total_wagered",
                          json_object_new_double(m_summary.total_wagered));
    json_object_object_add(jsummary, "total_profit",
                          json_object_new_double(m_summary.total_profit));
    json_object_object_add(jsummary, "roi",
                          json_object_new_double(calculateROI()));
    json_object_object_add(jsummary, "max_win",
                          json_object_new_double(m_summary.max_win));
    json_object_object_add(jsummary, "max_loss",
                          json_object_new_double(m_summary.max_loss));
    json_object_object_add(jsummary, "max_drawdown",
                          json_object_new_double(m_summary.max_drawdown));
    json_object_object_add(jsummary, "final_bankroll",
                          json_object_new_double(m_summary.final_bankroll));
    json_object_object_add(jsummary, "avg_true_count",
                          json_object_new_double(m_summary.avg_true_count));
    json_object_object_add(jsummary, "max_true_count",
                          json_object_new_double(m_summary.max_true_count));
    json_object_object_add(jsummary, "min_true_count",
                          json_object_new_double(m_summary.min_true_count));
    json_object_object_add(jsummary, "insurance_taken",
                          json_object_new_int(m_summary.insurance_taken));
    json_object_object_add(jsummary, "doubles",
                          json_object_new_int(m_summary.doubles));
    json_object_object_add(jsummary, "splits",
                          json_object_new_int(m_summary.splits));
    json_object_object_add(jsummary, "surrenders",
                          json_object_new_int(m_summary.surrenders));
    json_object_object_add(jsummary, "duration_seconds",
                          json_object_new_int64(m_summary.duration_seconds));
    json_object_object_add(jsummary, "strategy_adherence",
                          json_object_new_double(m_summary.strategy_adherence));

    return jsummary;
}

bool SessionRecorder::exportToJSON(const std::string& filepath) {
    auto& logger = Logger::getInstance();

    json_object* jroot = json_object_new_object();

    // Metadata
    json_object_object_add(jroot, "session_id",
                          json_object_new_string(m_sessionId.c_str()));
    json_object_object_add(jroot, "version",
                          json_object_new_string("2.5"));

    // Summary
    json_object_object_add(jroot, "summary", summaryToJSON());

    // Hands array
    json_object* jhands = json_object_new_array();
    for (const auto& hand : m_hands) {
        json_object_array_add(jhands, handToJSON(hand));
    }
    json_object_object_add(jroot, "hands", jhands);

    // Write to file
    const char* json_str = json_object_to_json_string_ext(
        jroot, JSON_C_TO_STRING_PRETTY);

    std::ofstream file(filepath);
    if (!file.is_open()) {
        logger.error("Failed to open file for writing: {}", filepath);
        json_object_put(jroot);
        return false;
    }

    file << json_str;
    file.close();

    json_object_put(jroot);

    logger.info("Session exported to JSON: {}", filepath);
    return true;
}

bool SessionRecorder::exportToCSV(const std::string& filepath) {
    auto& logger = Logger::getInstance();

    std::ofstream file(filepath);
    if (!file.is_open()) {
        logger.error("Failed to open CSV file for writing: {}", filepath);
        return false;
    }

    // Header
    file << "hand_number,player_cards,dealer_upcard,running_count,true_count,"
         << "recommended_action,actual_action,result,bet_amount,payout,timestamp_ms\n";

    // Data rows
    for (const auto& hand : m_hands) {
        // Join player cards with semicolon
        std::string player_cards_str;
        for (size_t i = 0; i < hand.player_cards.size(); i++) {
            player_cards_str += hand.player_cards[i];
            if (i < hand.player_cards.size() - 1) {
                player_cards_str += ";";
            }
        }

        file << hand.hand_number << ","
             << player_cards_str << ","
             << hand.dealer_upcard << ","
             << hand.running_count << ","
             << hand.true_count << ","
             << hand.recommended_action << ","
             << hand.actual_action << ","
             << hand.result << ","
             << hand.bet_amount << ","
             << hand.payout << ","
             << hand.timestamp_ms << "\n";
    }

    file.close();

    logger.info("Session exported to CSV: {}", filepath);
    return true;
}

float SessionRecorder::calculateWinRate() const {
    if (m_summary.total_hands == 0) return 0.0f;
    return (static_cast<float>(m_summary.wins) / m_summary.total_hands) * 100.0f;
}

float SessionRecorder::calculateROI() const {
    if (m_summary.total_wagered == 0.0) return 0.0f;
    return (m_summary.total_profit / m_summary.total_wagered) * 100.0f;
}

std::vector<double> SessionRecorder::getBankrollHistory() const {
    std::vector<double> history;
    double bankroll = m_startingBankroll;
    history.push_back(bankroll);

    for (const auto& hand : m_hands) {
        if (hand.result == "Win") {
            bankroll += hand.payout;
        } else if (hand.result == "Loss") {
            bankroll -= hand.bet_amount;
        }
        history.push_back(bankroll);
    }

    return history;
}

} // namespace utils
