// Created: October 30, 2025
// Location: include/game/ai/CouncilAI.h

#ifndef COUNCIL_AI_H
#define COUNCIL_AI_H

#include "game/ai/InformationPropagationSystem.h"
#include "core/types/game_types.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>

// Forward declarations
namespace game {
    namespace realm {
        enum class CouncilPosition : uint8_t;
    }
}

namespace AI {

// ============================================================================
// Council AI - Advisor AI for realm councils
// ============================================================================

class CouncilAI {
private:
    uint32_t m_actorId;
    ::game::types::EntityID m_realmId;
    std::string m_name;

    // Council composition
    std::unordered_map<game::realm::CouncilPosition, ::game::types::EntityID> m_councilors;

    // Voting history
    struct VoteRecord {
        std::string proposalType;
        bool votedFor;
        std::chrono::system_clock::time_point when;
    };
    std::vector<VoteRecord> m_votingHistory;

public:
    CouncilAI(
        uint32_t actorId,
        ::game::types::EntityID realmId,
        const std::string& name
    );

    void ProcessInformation(const InformationPacket& packet);

    // Council decisions
    bool ShouldApproveWar(::game::types::EntityID target) const;
    bool ShouldApproveTaxIncrease(float newRate) const;
    bool ShouldApproveAlliance(::game::types::EntityID ally) const;
    bool ShouldApproveSuccession(::game::types::EntityID heir) const;

    // Advisor recommendations
    std::vector<std::string> GetEconomicAdvice() const;
    std::vector<std::string> GetMilitaryAdvice() const;
    std::vector<std::string> GetDiplomaticAdvice() const;

    const std::string& GetName() const { return m_name; }
};

} // namespace AI

#endif // COUNCIL_AI_H
