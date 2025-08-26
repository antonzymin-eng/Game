#pragma once
#include <string>

namespace core {
class SimulationState;
}

namespace core::loaders {

// Loads data from ./assets/data/*.csv if present; otherwise keeps existing values or seeds defaults.
void loadAll(SimulationState& sim);

// Individual loaders (CSV); safe to call repeatedly.
bool loadFactionsCSV(SimulationState& sim, const std::string& path = "./assets/data/factions.csv");
bool loadMarketCSV  (SimulationState& sim, const std::string& path = "./assets/data/market.csv");
bool loadAgentsCSV  (SimulationState& sim, const std::string& path = "./assets/data/agents.csv");

} // namespace core::loaders