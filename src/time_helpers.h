#ifndef __TIME_HELPERS__
#define __TIME_HELPERS__

#include "Arduino.h"

/// @brief try to contact the ntp server to get the absolute time
/// requires a WiFi connection
bool tryToUpdateAbsoluteTime();

/// @brief if possible, fill the [buffer] with a formatter date/time string
/// returns false if no NTP reference time is available
bool getLocalTimeString(char *buffer, uint32_t bufferSize);

uint32_t getEpochTime();

#endif