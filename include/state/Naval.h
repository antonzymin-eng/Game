#pragma once
#include <vector>

namespace core {

struct NavalShip {
    int id = 0;
    int strength = 0;
};

struct NavalFleet {
    int id = 0;
    int location = 0;
    std::vector<NavalShip> ships;
};

} // namespace core
