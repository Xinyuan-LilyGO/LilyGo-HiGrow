#include "Arduino.h"

// a structure that holds a unit [time] and the corresponding value of millis() when it was taken
struct TmAndMillis
{
    tm time;
    uint32_t time_ms;
};

/// @brief try to contact the ntp server to get the absolute time
/// requires a WiFi connection
bool tryToGetAbsoluteTime();

/// @brief return a structure that has a absolute time object and corresponding value of millis() at that time
/// The pair of values can be used to convert a millis() result to a absolute time
const TmAndMillis &absoluteTimeReference();

/// @brief convert a millis result to a tm object using a reference time point pair
tm millisToTm(uint32_t millis, const TmAndMillis &referenceTime);

/// @brief convert a millis result to a timestamp since epoch, using a reference time point pair
uint32_t millisToEpoch(uint32_t millis, const TmAndMillis &referenceTime);

/// @brief if possible, fill the [buffer] with a formatter date/time string
/// returns false if no NTP reference time is available
bool getLocalTimeString(char *buffer, uint32_t bufferSize);

/// @brief has an absolute time reference been obtained?
bool hasAbsoluteTimeReference();