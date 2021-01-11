#include "time_helpers.h"

constexpr long kGmtOffset_s = 0;        // offset between GMT and your local time
constexpr int kDaylightOffset_s = 3600; // offset for daylight saving
constexpr char kNtpServer[] = "pool.ntp.org";
TmAndMillis g_globalTimeReference;

bool tryToGetGlobalTime()
{
    configTime(kGmtOffset_s, kDaylightOffset_s, kNtpServer);
    if (!getLocalTime(&g_globalTimeReference.time))
    {
        Serial.println("Failed to get local time");
        return false;
    }

    g_globalTimeReference.time_ms = millis();
    return true;
}

const TmAndMillis& globalTimeReference()
{
    return g_globalTimeReference;
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