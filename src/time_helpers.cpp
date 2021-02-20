#include "time_helpers.h"
#include "driver/timer.h"

namespace
{
    constexpr long kGmtOffset_s = 0;        // offset between GMT and your local time
    constexpr int kDaylightOffset_s = 3600; // offset for daylight saving
    constexpr char kNtpServer[] = "pool.ntp.org";
}

bool tryToUpdateAbsoluteTime()
{
    configTime(kGmtOffset_s, kDaylightOffset_s, kNtpServer);
    tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to get local time");
        return false;
    }
    return true;
}

bool getLocalTimeString(char *buffer, uint32_t bufferSize)
{
    tm time;
    if (!getLocalTime(&time))
    {
        return false;
    }
    strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", &time);
    return true;
}

uint32_t getEpochTime()
{
    tm time;
    if (!getLocalTime(&time))
    {
        return 0;
    }
    const time_t epochTime = mktime(&time);
    return static_cast<uint32_t>(epochTime);
}