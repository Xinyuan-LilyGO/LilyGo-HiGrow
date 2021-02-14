#include "Arduino.h"

// a structure that holds a unit [time] and the corresponding value of millis() when it was taken
struct TmAndMillis
{
    tm time;
    uint32_t time_ms;
};

/// @brief try to contact the ntp server to get the global time
/// requires a WiFi connection
bool tryToGetGlobalTime();

/// @brief return a structure that has a global time object and corresponding value of millis() at that time
/// The pair of values can be used to convert a millis() result to a global time
const TmAndMillis& globalTimeReference();

/// @brief convery a millis result to a tm object using a reference time point pair
tm millisToTm(uint32_t millis, const TmAndMillis &referenceTime);

uint32_t formatTimeAsNumber();