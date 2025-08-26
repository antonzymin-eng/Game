#pragma once
#include <string>
#include <vector>

namespace core {

struct Army {
    int id = 0;
    int owner = 0;
    int location = 0;
    int strength = 0;
    int morale = 0;
};

struct Faction {
    int id = 0;
    std::string name;
    float power = 0.0f;
    int capitalProvince = 0;
    int treasury = 0;
};

struct Agent {
    int id = 0;
    std::string name;
    std::string location;
    std::string mission;
    std::string status;
};

struct MarketGood {
    std::string name;
    float price   = 0.0f;
    float supply  = 0.0f;
    float demand  = 0.0f;
};

struct Market {
    std::vector<MarketGood> goods;

    void clear() { goods.clear(); }
    void push_back(const MarketGood& g) { goods.push_back(g); }
    bool empty() const { return goods.empty(); }
    size_t size() const { return goods.size(); }
    MarketGood& operator[](size_t i) { return goods[i]; }
    const MarketGood& operator[](size_t i) const { return goods[i]; }
    auto begin() { return goods.begin(); }
    auto end()   { return goods.end(); }
    auto begin() const { return goods.begin(); }
    auto end()   const { return goods.end(); }
};

struct PopulationRegion {
    int provinceId = 0;
    std::string culture;
    float unrest = 0.0f;
};

} // namespace core
