#pragma once
namespace core {
struct GameClock {
    int year = 1600;
    int month = 1;
    int day = 1;
    int hour = 0;
    long long ticks = 0;
    void advanceHours(int hours);
};
} // namespace core
