#pragma once
#include <string>

namespace core {

struct TradeRoute {
    int origin = 0;
    int dest = 0;
    float throughput = 0.0f;
    bool disrupted = false;
    std::string name;
};

} // namespace core
