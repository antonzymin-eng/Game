#include "state/Loaders.h"
#include "state/SimulationState.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>

namespace core::loaders {

static inline std::vector<std::string> splitCSV(const std::string& line) {
    std::vector<std::string> out; out.reserve(8);
    std::string cur; bool inQuote=false;
    for (char c : line) {
        if (c=='"') { inQuote = !inQuote; }
        else if (c==',' && !inQuote) { out.push_back(cur); cur.clear(); }
        else { cur.push_back(c); }
    }
    out.push_back(cur);
    return out;
}
static inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b-a+1);
}
static inline int toInt(const std::string& s) { return std::atoi(s.c_str()); }
static inline long long toLL(const std::string& s) { return std::atoll(s.c_str()); }
static inline double toD(const std::string& s) { return std::atof(s.c_str()); }
static inline float toF(const std::string& s) { return (float)std::atof(s.c_str()); }

bool loadFactionsCSV(core::SimulationState& sim, const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    auto& vec = sim.factions();
    vec.clear();
    // expected header: id,name,power
    std::getline(f, line);
    while (std::getline(f, line)) {
        auto cols = splitCSV(line);
        if (cols.size() < 3) continue;
        core::Faction x;
        x.id = toInt(trim(cols[0]));
        x.name = trim(cols[1]);
        x.power = toF(trim(cols[2]));
        vec.push_back(x);
    }
    return !vec.empty();
}

bool loadMarketCSV(core::SimulationState& sim, const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    auto& vec = sim.market();
    vec.clear();
    // expected header: name,price,supply,demand
    std::getline(f, line);
    while (std::getline(f, line)) {
        auto cols = splitCSV(line);
        if (cols.size() < 4) continue;
        core::MarketGood x;
        x.name   = trim(cols[0]);
        x.price  = toD(trim(cols[1]));
        x.supply = toD(trim(cols[2]));
        x.demand = toD(trim(cols[3]));
        vec.push_back(x);
    }
    return !vec.empty();
}

bool loadAgentsCSV(core::SimulationState& sim, const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    auto& vec = sim.agents();
    vec.clear();
    // expected header: id,name,location,mission,status
    std::getline(f, line);
    while (std::getline(f, line)) {
        auto cols = splitCSV(line);
        if (cols.size() < 5) continue;
        core::Agent a;
        a.id       = toInt(trim(cols[0]));
        a.name     = trim(cols[1]);
        a.location = trim(cols[2]);
        a.mission  = trim(cols[3]);
        a.status   = trim(cols[4]);
        vec.push_back(a);
    }
    return !vec.empty();
}

void loadAll(core::SimulationState& sim) {
    bool okF = loadFactionsCSV(sim);
    bool okM = loadMarketCSV(sim);
    bool okA = loadAgentsCSV(sim);
    (void)okF; (void)okM; (void)okA;
}

} // namespace core::loaders