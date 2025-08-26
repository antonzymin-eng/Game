#pragma once
#include <string>

// Forward declaration in the correct namespace
namespace core { class SimulationState; }

namespace persist {
    // Save to latest slot; returns true on success. If outPath is non-null, the final path is written there.
    bool SaveLatest(const core::SimulationState& state, std::string* outPath = nullptr);

    // Load from latest slot; returns true on success. If outPath is non-null, the loaded path is written there.
    bool LoadLatest(core::SimulationState& state, std::string* outPath = nullptr);
} // namespace persist
