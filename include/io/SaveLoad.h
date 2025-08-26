
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace io {

struct SaveEntry {
    std::string name;
    std::string fullpath;
};

struct SaveLoadHooks {
    std::function<std::string()> serialize_cb;
    std::function<bool(const std::string&)> deserialize_cb;
    std::function<void()> on_post_load_cb;
};

class SaveLoadManager {
public:
    static SaveLoadManager& I();
    void setHooks(const SaveLoadHooks& h);
    std::vector<SaveEntry> listSaves() const;
    bool deleteSave(const std::string& name) const;
    bool saveAs(const std::string& name) const;
    bool quickSave() const;
    bool loadByName(const std::string& name) const;
    std::optional<SaveEntry> latestSave() const;
    bool loadLatest() const;
    static std::string SaveDir();
private:
    SaveLoadHooks hooks_;
};

} // namespace io
