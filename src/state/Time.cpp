#include "state/Time.h"

namespace core {

static int DaysInMonth(int y, int m) {
    static const int md[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int d = md[(m - 1) % 12];
    bool leap = (y % 4 == 0) && ((y % 100 != 0) || (y % 400 == 0));
    if (m == 2 && leap) d = 29;
    return d;
}

void GameClock::advanceHours(int hours) {
    ticks += hours;
    hour += hours;
    while (hour >= 24) {
        hour -= 24;
        day += 1;
        if (day > DaysInMonth(year, month)) {
            day = 1;
            month += 1;
            if (month > 12) {
                month = 1;
                year += 1;
            }
        }
    }
}

} // namespace core
