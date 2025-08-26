
#include "io/SaveLoad.h"
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace io {
namespace fs = std::filesystem;

static fs::path saves_dir() { return fs::path("saves"); }

SaveLoadManager& SaveLoadManager::I() { static SaveLoadManager inst; return inst; }
void SaveLoadManager::setHooks(const SaveLoadHooks& h) { hooks_ = h; }

std::vector<SaveEntry> SaveLoadManager::listSaves() const {
    std::vector<SaveEntry> out;
    std::error_code ec;
    fs::create_directories(saves_dir(), ec);
    for (auto& p : fs::directory_iterator(saves_dir(), ec)) {
        if (!p.is_regular_file()) continue;
        if (p.path().extension() == ".misave") out.push_back({p.path().filename().string(), p.path().string()});
    }
    std::sort(out.begin(), out.end(), [](const SaveEntry& a, const SaveEntry& b){ return a.name < b.name; });
    return out;
}

bool SaveLoadManager::deleteSave(const std::string& name) const {
    std::error_code ec;
    return fs::remove(saves_dir() / name, ec);
}

bool SaveLoadManager::saveAs(const std::string& name) const {
    fs::create_directories(saves_dir());
    std::string outfile = name;
    if (fs::path(outfile).extension() != ".misave") outfile += ".misave";
    std::ofstream out((saves_dir() / outfile).string(), std::ios::binary);
    if (!out) return false;
    std::string data = hooks_.serialize_cb ? hooks_.serialize_cb() : std::string("EMPTY_SAVE");
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(out);
}

bool SaveLoadManager::quickSave() const { return saveAs("quick.misave"); }

bool SaveLoadManager::loadByName(const std::string& name) const {
    std::ifstream in((saves_dir() / name).string(), std::ios::binary);
    if (!in) return false;
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    bool ok = true;
    if (hooks_.deserialize_cb) ok = hooks_.deserialize_cb(data);
    if (ok && hooks_.on_post_load_cb) hooks_.on_post_load_cb();
    return ok;
}

std::optional<SaveEntry> SaveLoadManager::latestSave() const {
    std::optional<SaveEntry> best;
    std::error_code ec;
    fs::file_time_type best_time{};
    for (auto& p : fs::directory_iterator(saves_dir(), ec)) {
        if (!p.is_regular_file() || p.path().extension() != ".misave") continue;
        auto t = fs::last_write_time(p.path(), ec);
        if (!best || t > best_time) {
            best = SaveEntry{ p.path().filename().string(), p.path().string() };
            best_time = t;
        }
    }
    return best;
}

bool SaveLoadManager::loadLatest() const {
    auto best = latestSave();
    if (!best) return false;
    std::ifstream in(best->fullpath, std::ios::binary);
    if (!in) return false;
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    bool ok = true;
    if (hooks_.deserialize_cb) ok = hooks_.deserialize_cb(data);
    if (ok && hooks_.on_post_load_cb) hooks_.on_post_load_cb();
    return ok;
}

std::string SaveLoadManager::SaveDir() { return saves_dir().string(); }

} // namespace io
