#include "basic_strategy.hpp"
#include "../../utils/logger.hpp"
#include <unordered_map>

namespace intelligence {

BasicStrategy::BasicStrategy() {
    buildStrategyTables();
}

BasicStrategy::~BasicStrategy() = default;

void BasicStrategy::initialize(const std::string& rules) {
    m_rules = rules;
    auto& logger = utils::Logger::getInstance();
    logger.info("Initializing basic strategy with rules: {}", rules);

    buildStrategyTables();

    if (m_deviationsEnabled) {
        loadIllustrious18();
        loadFab4();
        logger.info("Loaded Illustrious 18 and Fab 4 deviations");
    }
}

void BasicStrategy::buildStrategyTables() {
    // Initialize all entries to Hit (default)
    for (int i = 0; i < 22; i++) {
        for (int j = 0; j < 13; j++) {
            m_hardTotals[i][j] = Action::Hit;
            m_softTotals[i][j] = Action::Hit;
            m_pairSplitting[i][j] = Action::Hit;
        }
    }

    // Build Hard Totals Strategy (S17 with DAS)
    // Player hard totals 4-21 vs Dealer upcards 2-11 (Ace=11)

    // Hard 17-21: Always Stand
    for (int total = 17; total <= 21; total++) {
        for (int dealer = 2; dealer <= 11; dealer++) {
            m_hardTotals[total][dealer] = Action::Stand;
        }
    }

    // Hard 13-16: Stand vs 2-6, Hit vs 7-A
    for (int total = 13; total <= 16; total++) {
        for (int dealer = 2; dealer <= 6; dealer++) {
            m_hardTotals[total][dealer] = Action::Stand;
        }
        for (int dealer = 7; dealer <= 11; dealer++) {
            m_hardTotals[total][dealer] = Action::Hit;
        }
    }

    // Hard 12: Stand vs 4-6, Hit otherwise
    m_hardTotals[12][4] = Action::Stand;
    m_hardTotals[12][5] = Action::Stand;
    m_hardTotals[12][6] = Action::Stand;

    // Hard 11: Double vs all, Hit if can't double
    for (int dealer = 2; dealer <= 11; dealer++) {
        m_hardTotals[11][dealer] = Action::Double;
    }

    // Hard 10: Double vs 2-9, Hit vs 10/A
    for (int dealer = 2; dealer <= 9; dealer++) {
        m_hardTotals[10][dealer] = Action::Double;
    }

    // Hard 9: Double vs 3-6, Hit otherwise
    for (int dealer = 3; dealer <= 6; dealer++) {
        m_hardTotals[9][dealer] = Action::Double;
    }

    // Build Soft Totals Strategy (A,2 through A,9)
    // Soft hands: A+2=13, A+3=14, ... A+9=20

    // Soft 19-20 (A,8 and A,9): Always Stand
    for (int dealer = 2; dealer <= 11; dealer++) {
        m_softTotals[19][dealer] = Action::Stand;
        m_softTotals[20][dealer] = Action::Stand;
    }

    // Soft 18 (A,7): Double vs 2-6, Stand vs 7-8, Hit vs 9-A
    for (int dealer = 2; dealer <= 6; dealer++) {
        m_softTotals[18][dealer] = Action::Double;
    }
    m_softTotals[18][7] = Action::Stand;
    m_softTotals[18][8] = Action::Stand;
    m_softTotals[18][9] = Action::Hit;
    m_softTotals[18][10] = Action::Hit;
    m_softTotals[18][11] = Action::Hit;

    // Soft 17 (A,6): Double vs 3-6, Hit otherwise
    for (int dealer = 3; dealer <= 6; dealer++) {
        m_softTotals[17][dealer] = Action::Double;
    }

    // Soft 15-16 (A,4 and A,5): Double vs 4-6, Hit otherwise
    for (int dealer = 4; dealer <= 6; dealer++) {
        m_softTotals[15][dealer] = Action::Double;
        m_softTotals[16][dealer] = Action::Double;
    }

    // Soft 13-14 (A,2 and A,3): Double vs 5-6, Hit otherwise
    m_softTotals[13][5] = Action::Double;
    m_softTotals[13][6] = Action::Double;
    m_softTotals[14][5] = Action::Double;
    m_softTotals[14][6] = Action::Double;

    // Build Pair Splitting Strategy (with DAS - Double After Split)

    // Always split A,A and 8,8
    for (int dealer = 2; dealer <= 11; dealer++) {
        m_pairSplitting[11][dealer] = Action::Split; // A,A
        m_pairSplitting[8][dealer] = Action::Split;  // 8,8
    }

    // Never split 10,10, 5,5, 4,4
    // (already initialized to Hit, will be handled by hard total logic)

    // 9,9: Split vs 2-9 except 7, Stand vs 7,10,A
    for (int dealer = 2; dealer <= 9; dealer++) {
        if (dealer != 7) {
            m_pairSplitting[9][dealer] = Action::Split;
        }
    }
    m_pairSplitting[9][7] = Action::Stand;
    m_pairSplitting[9][10] = Action::Stand;
    m_pairSplitting[9][11] = Action::Stand;

    // 7,7: Split vs 2-7, Hit vs 8-A
    for (int dealer = 2; dealer <= 7; dealer++) {
        m_pairSplitting[7][dealer] = Action::Split;
    }

    // 6,6: Split vs 2-6 (with DAS), Hit vs 7-A
    for (int dealer = 2; dealer <= 6; dealer++) {
        m_pairSplitting[6][dealer] = Action::Split;
    }

    // 3,3 and 2,2: Split vs 2-7 (with DAS), Hit vs 8-A
    for (int dealer = 2; dealer <= 7; dealer++) {
        m_pairSplitting[3][dealer] = Action::Split;
        m_pairSplitting[2][dealer] = Action::Split;
    }
}

void BasicStrategy::loadIllustrious18() {
    // The Illustrious 18 are index plays stored separately
    // They will be applied in getDeviationAction() based on true count

    // This is a marker function - actual deviations are in getDeviationAction()
    auto& logger = utils::Logger::getInstance();
    logger.info("Illustrious 18 deviations ready");
}

void BasicStrategy::loadFab4() {
    // The Fab 4 are surrender index plays
    // They will be applied in getDeviationAction() based on true count

    auto& logger = utils::Logger::getInstance();
    logger.info("Fab 4 surrender deviations ready");
}

Action BasicStrategy::getAction(uint32_t playerTotal,
                                core::CardRank dealerUpcard,
                                bool isSoft,
                                bool canDouble,
                                bool canSplit) const {
    int dealerValue = static_cast<int>(dealerUpcard);
    if (dealerValue == 1) dealerValue = 11; // Ace

    // Validate inputs
    if (playerTotal < 4 || playerTotal > 21) return Action::Stand;
    if (dealerValue < 2 || dealerValue > 11) return Action::Stand;

    Action action;

    if (canSplit) {
        // Check pair splitting table first
        int pairValue = playerTotal / 2;
        action = m_pairSplitting[pairValue][dealerValue];
        if (action == Action::Split) return action;
    }

    if (isSoft) {
        action = m_softTotals[playerTotal][dealerValue];
    } else {
        action = m_hardTotals[playerTotal][dealerValue];
    }

    // If Double is recommended but not allowed, Hit instead
    if (action == Action::Double && !canDouble) {
        return Action::Hit;
    }

    return action;
}

Action BasicStrategy::getDeviationAction(uint32_t playerTotal,
                                        core::CardRank dealerUpcard,
                                        float trueCount) const {
    if (!m_deviationsEnabled) {
        // Deviations disabled, use basic strategy
        return getAction(playerTotal, dealerUpcard, false, true, false);
    }

    int dealerValue = static_cast<int>(dealerUpcard);
    if (dealerValue == 1) dealerValue = 11; // Ace

    // THE ILLUSTRIOUS 18 - Deviations based on True Count
    // These provide 80-85% of the value of all index plays

    // 1. Insurance at TC >= +3 (most important)
    // (Handled separately in game logic)

    // 2. 16 vs 10: Stand at TC >= 0 (instead of Hit/Surrender)
    if (playerTotal == 16 && dealerValue == 10) {
        if (trueCount >= 0.0f) return Action::Stand;
        return Action::Hit;
    }

    // 3. 15 vs 10: Surrender at TC >= 0
    if (playerTotal == 15 && dealerValue == 10) {
        if (trueCount >= 0.0f) return Action::Surrender;
        return Action::Hit;
    }

    // 4. 16 vs 9: Stand at TC >= +5
    if (playerTotal == 16 && dealerValue == 9) {
        if (trueCount >= 5.0f) return Action::Stand;
        return Action::Hit;
    }

    // 5. 13 vs 2: Stand at TC >= -1
    if (playerTotal == 13 && dealerValue == 2) {
        if (trueCount >= -1.0f) return Action::Stand;
        return Action::Hit;
    }

    // 6. 13 vs 3: Stand at TC >= -2
    if (playerTotal == 13 && dealerValue == 3) {
        if (trueCount >= -2.0f) return Action::Stand;
        return Action::Hit;
    }

    // 7. 11 vs A: Double at TC >= +1
    if (playerTotal == 11 && dealerValue == 11) {
        if (trueCount >= 1.0f) return Action::Double;
        return Action::Hit;
    }

    // 8. 10 vs 10: Double at TC >= +4
    if (playerTotal == 10 && dealerValue == 10) {
        if (trueCount >= 4.0f) return Action::Double;
        return Action::Hit;
    }

    // 9. 10 vs A: Double at TC >= +4
    if (playerTotal == 10 && dealerValue == 11) {
        if (trueCount >= 4.0f) return Action::Double;
        return Action::Hit;
    }

    // 10. 9 vs 2: Double at TC >= +1
    if (playerTotal == 9 && dealerValue == 2) {
        if (trueCount >= 1.0f) return Action::Double;
        return Action::Hit;
    }

    // 11. 9 vs 7: Double at TC >= +3
    if (playerTotal == 9 && dealerValue == 7) {
        if (trueCount >= 3.0f) return Action::Double;
        return Action::Hit;
    }

    // 12. 12 vs 3: Stand at TC >= +2
    if (playerTotal == 12 && dealerValue == 3) {
        if (trueCount >= 2.0f) return Action::Stand;
        return Action::Hit;
    }

    // 13. 12 vs 2: Stand at TC >= +3
    if (playerTotal == 12 && dealerValue == 2) {
        if (trueCount >= 3.0f) return Action::Stand;
        return Action::Hit;
    }

    // 14. 10,10 vs 5: Split at TC >= +5
    // (Handled in pair splitting logic)

    // 15. 10,10 vs 6: Split at TC >= +4
    // (Handled in pair splitting logic)

    // 16. 15 vs A (H17): Stand at TC >= +1 (H17 rules)
    if (playerTotal == 15 && dealerValue == 11) {
        if (m_rules.find("h17") != std::string::npos) {
            if (trueCount >= 1.0f) return Action::Stand;
        }
        return Action::Hit;
    }

    // 17. 16 vs A: Stand at TC >= +2 (H17 rules)
    if (playerTotal == 16 && dealerValue == 11) {
        if (trueCount >= 2.0f) return Action::Stand;
        return Action::Hit;
    }

    // 18. 12 vs 4: Stand at TC >= 0
    if (playerTotal == 12 && dealerValue == 4) {
        if (trueCount >= 0.0f) return Action::Stand;
        return Action::Hit;
    }

    // FAB 4 - Surrender Deviations
    // These are the most valuable surrender index plays

    // 1. 14 vs 10: Surrender at TC >= +3
    if (playerTotal == 14 && dealerValue == 10) {
        if (trueCount >= 3.0f) return Action::Surrender;
        return Action::Hit;
    }

    // 2. 15 vs 9: Surrender at TC >= +2
    if (playerTotal == 15 && dealerValue == 9) {
        if (trueCount >= 2.0f) return Action::Surrender;
        return Action::Hit;
    }

    // 3. 15 vs 10: Surrender at TC >= 0 (already covered above in Illustrious 18)

    // 4. 15 vs A: Surrender based on rules
    if (playerTotal == 15 && dealerValue == 11) {
        if (m_rules.find("s17") != std::string::npos) {
            if (trueCount >= 1.0f) return Action::Surrender;
        } else if (m_rules.find("h17") != std::string::npos) {
            if (trueCount >= -1.0f) return Action::Surrender;
        }
        return Action::Hit;
    }

    // No deviation applies, use basic strategy
    return getAction(playerTotal, dealerUpcard, false, true, false);
}

} // namespace intelligence
