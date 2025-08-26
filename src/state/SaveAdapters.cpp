#include "state/SaveAdapters.h"
#include <sstream>

namespace core {

static std::string SerializeSim() {
    // Minimal JSON-ish dump of some state; extend as needed.
    auto& sim = SimulationState::I();
    std::ostringstream os;
    os << "{\n";
    os << "  \"factions\": [\n";
    for (size_t i=0;i<sim.factions().size();++i) {
        const auto& f = sim.factions()[i];
        os << "    {\"id\":"<<f.id<<",\"name\":\""<<f.name<<"\",\"power\":"<<f.power<<"}";
        if (i+1<sim.factions().size()) os << ",";
        os << "\n";
    }
    os << "  ]\n";
    os << "}\n";
    return os.str();
}

static bool DeserializeSim(const std::string& blob) {
    // Stub: accept any blob and set a basic world so loads "work".
    auto& sim = SimulationState::I();
    sim.factions().clear();
    sim.factions().push_back({1,"Alpha",100});
    sim.factions().push_back({2,"Beta",90});
    return true;
}

static void AfterLoad() {
    // Placeholder hook — e.g., rebuild caches, notify systems, etc.
}

void InstallSaveLoadAdapters() {
    io::SaveLoadHooks hooks;
    hooks.serialize_cb   = &SerializeSim;
    hooks.deserialize_cb = &DeserializeSim;
    hooks.on_post_load_cb= &AfterLoad;
    io::SaveLoadManager::I().setHooks(hooks);
}

} // namespace core
