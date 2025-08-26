// src/persistence/SaveSystem.cpp
// Persistence helpers backing the functions declared in persistence/SaveSystem.h

#include <string>
#include "persistence/SaveSystem.h"
#include "state/SimulationState.h"
#include "io/SaveLoad.h"

namespace persist {

// Save to the default 'quick' slot.
bool SaveLatest(const core::SimulationState& /*state*/, std::string* outPath) {
    auto& mgr = io::SaveLoadManager::I();
    const bool ok = mgr.quickSave();
    if (ok && outPath) {
        *outPath = io::SaveLoadManager::SaveDir();
    #ifdef _WIN32
        *outPath += "\\quick.json";
    #else
        *outPath += "/quick.json";
    #endif
    }
    return ok;
}

// Load from the default 'quick' slot.
bool LoadLatest(core::SimulationState& /*state*/, std::string* outPath) {
    auto& mgr = io::SaveLoadManager::I();
    if (outPath) {
        *outPath = io::SaveLoadManager::SaveDir();
    #ifdef _WIN32
        *outPath += "\\quick.json";
    #else
        *outPath += "/quick.json";
    #endif
    }
    return mgr.loadByName("quick");
}

} // namespace persist
