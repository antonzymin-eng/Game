#include "state/SimulationState.h"

namespace core {

SimulationState& SimulationState::I() {
    static SimulationState s;
    return s;
}

std::string SimulationState::factionName(int id) const {
    for (const auto& f : factions_) if (f.id == id) return f.name;
    return std::string("Faction ") + std::to_string(id);
}

} // namespace core
