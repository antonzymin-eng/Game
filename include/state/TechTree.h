#pragma once
#include <string>
#include <vector>

namespace core {

struct TechNode {
    int id = 0;
    std::string name;
    bool unlocked = false;
    std::vector<int> prereqs;
};

} // namespace core
