#pragma once

#include "../../core/types.hpp"

namespace intelligence {

enum class Action {
    Hit,
    Stand,
    Double,
    Split,
    Surrender
};

class BasicStrategy {
public:
    BasicStrategy();
    ~BasicStrategy();

    void initialize(const std::string& rules);
    
    Action getAction(uint32_t playerTotal, 
                    core::CardRank dealerUpcard,
                    bool isSoft,
                    bool canDouble,
                    bool canSplit) const;
    
    Action getDeviationAction(uint32_t playerTotal,
                             core::CardRank dealerUpcard,
                             float trueCount) const;

private:
    void buildStrategyTables();
    void loadIllustrious18();
    void loadFab4();
    
    // Strategy lookup tables [player total][dealer upcard]
    Action m_hardTotals[22][13];
    Action m_softTotals[22][13];
    Action m_pairSplitting[13][13];
    
    bool m_deviationsEnabled{true};
    std::string m_rules;
};

} // namespace intelligence
