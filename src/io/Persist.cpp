// src/io/Persist.cpp
#include "io/SaveLoad.h"
#include "state/SimulationState.h"

#include <filesystem>
#include <optional>
#include <string>

namespace fs = std::filesystem;

namespace persist {

    // Pick the most recently modified entry in the save directory
    static std::optional<io::SaveEntry> latest_entry() {
        const auto entries = io::SaveLoadManager::I().listSaves();
        if (entries.empty()) return std::nullopt;

        std::optional<io::SaveEntry> best;
        fs::file_time_type best_time{};

        std::error_code ec;
        for (const auto& e : entries) {
            const auto t = fs::last_write_time(e.fullpath, ec);
            if (ec) continue;
            if (!best || t > best_time) {
                best = e;
                best_time = t;
            }
        }
        return best;
    }

    // Save to the quick slot (your manager decides the actual name),
    // and return the full path of the freshest file we can see.
    bool SaveLatest(const core::SimulationState& /*sim*/, std::string* out_path) {
        const bool ok = io::SaveLoadManager::I().quickSave();
        if (!ok) return false;

        if (out_path) {
            if (auto le = latest_entry()) {
                *out_path = le->fullpath;
            }
            else {
                // Fallback guess: saves/quick.misave (in case listSaves() is empty)
                *out_path = (fs::path(io::SaveLoadManager::SaveDir()) / "quick.misave").string();
            }
        }
        return true;
    }

    // Load the newest save we can find via listSaves() timestamps.
    bool LoadLatest(core::SimulationState& /*sim*/, std::string* out_path) {
        auto le = latest_entry();
        if (!le) return false;

        if (out_path) *out_path = le->fullpath;
        return io::SaveLoadManager::I().loadByName(le->name);
    }

} // namespace persist
