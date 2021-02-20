#include "time_helpers.h"

namespace
{
    constexpr long kGmtOffset_s = 0;        // offset between GMT and your local time
    constexpr int kDaylightOffset_s = 3600; // offset for daylight saving
    constexpr char kNtpServer[] = "pool.ntp.org";
    TmAndMillis g_absoluteTimeReference;
    bool g_hasAbsolutelTimeReference = false;
}

bool tryToGetAbsoluteTime()
{
    configTime(kGmtOffset_s, kDaylightOffset_s, kNtpServer);
    if (!getLocalTime(&g_absoluteTimeReference.time))
    {
        Serial.println("Failed to get local time");
        g_hasAbsolutelTimeReference = false;
        return false;
    }

    g_absoluteTimeReference.time_ms = millis();
    g_hasAbsolutelTimeReference = true;
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

bool hasAbsoluteTimeReference()
{
    return g_hasAbsolutelTimeReference;
}

const TmAndMillis &absoluteTimeReference()
{
    return g_absoluteTimeReference;
}

tm millisToTm(uint32_t millis, const TmAndMillis &referenceTime)
{
    int64_t diff_ms = (int64_t)millis - referenceTime.time_ms;
    int64_t diff_s = diff_ms / 1000;
    int64_t diffPart_hrs = diff_s / 3600;
    diff_s -= diffPart_hrs * 3600;
    int64_t diffPart_mins = diff_s / 60;
    diff_s -= diffPart_mins * 60;
    int64_t diffPart_s = diff_s;

    tm timeG = referenceTime.time;
    int hours = timeG.tm_hour + diffPart_hrs;
    int mins = timeG.tm_min + diffPart_mins;
    int secs = timeG.tm_sec + diffPart_s;

    // some parts might have overflowed so need to adjust
    int64_t extraMins = secs / 60;
    mins += extraMins;
    secs -= extraMins * 60;

    int64_t extraHours = mins / 60;
    hours += extraHours;
    hours %= 24;
    mins -= extraHours * 60;

    return timeG;
}

uint32_t millisToEpoch(uint32_t millis, const TmAndMillis &referenceTime)
{
    tm time = millisToTm(millis, referenceTime);
    const time_t epochTime = mktime(&time);
    return static_cast<uint32_t>(epochTime);
}