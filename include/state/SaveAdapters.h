#pragma once
#include "io/SaveLoad.h"
#include "state/SimulationState.h"

namespace core {
// Wires game serialization into io::SaveLoadManager via hooks.
// Call once during startup.
void InstallSaveLoadAdapters();
} // namespace core
