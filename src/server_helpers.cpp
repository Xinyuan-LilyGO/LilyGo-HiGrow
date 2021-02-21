#include "server_helpers.h"
#include "HttpClient.h"
#include <ESPmDNS.h>

#define xstr(s) str(s)
#define str(s) #s

#define USER_AGENT "TTGO-Sensor (" str(FW_VERSION_MAJOR) "." str(FW_VERSION_MINOR) "." str(FW_VERSION_PATCH) ")"

namespace
{
    bool getNextSensorName(HttpClient *httpClient, const char *serverAddress, uint16_t serverPort, const char *apiPath, char *outSensorName, uint8_t bufferLength)
    {
        int error = 0;

        Serial.print("Performing GET request to: ");
        Serial.print(serverAddress);
        Serial.println(apiPath);
        error = httpClient->get(serverAddress, serverPort, apiPath, USER_AGENT);
        if (error != 0)
        {
            Serial.println("HTTP connection failed");
            return false;
        }

        const int httpResponseCode = httpClient->responseStatusCode();
        Serial.print("HTTP Code: ");
        Serial.println(httpResponseCode);

        if (httpResponseCode != 200)
        {
            return false;
        }

        error = httpClient->skipResponseHeaders();
        if (error != HTTP_SUCCESS)
        {
            return false;
        }

        // read response
        const int bodyLength = httpClient->contentLength();
        int remaining = bodyLength;
        constexpr uint32_t kTimeout_ms = 10 * 1000;
        memset(outSensorName, 0, bufferLength);
        uint32_t bufferPos = 0;
        uint32_t timeout_ms = millis();
        while (remaining > 0                                           //
               && (httpClient->connected() || httpClient->available()) //
               && ((millis() - timeout_ms) < kTimeout_ms))             //
        {
            if (httpClient->available())
            {
                outSensorName[bufferPos] = httpClient->read();
                ++bufferPos;
                --remaining;

                if (bufferPos == bufferLength)
                {
                    Serial.println("Buffer filed");
                    break;
                }
            }
            else
            {
                // delay a little until we get data
                delay(1000);
            }
        }

        if (remaining > 0)
        {
            Serial.print("Expected ");
            Serial.print(remaining);
            Serial.println(" more characters");
        }

        // response is json, so strip the " from the start and end
        // copy from position 1 to 0, and replace trailing " with 0
        const size_t len = strlen(outSensorName);
        strcpy(outSensorName, outSensorName + 1);
        outSensorName[len - 2] = 0;

        return true;
    }
}

bool getNextSensorName(WiFiClient *wifiClient,    //
                       const char *serverAddress, //
                       uint16_t serverPort,       //
                       const char *apiPath,       //
                       char *outSensorName,       //
                       uint8_t bufferLength,      //
                       bool mdnsLookup)
{
    HttpClient httpClient(*wifiClient);
    bool success = false;
    if (mdnsLookup)
    {
        mdns_init();
        const IPAddress hostAddress = MDNS.queryHost(serverAddress);
        if (hostAddress == IPAddress())
        {
            httpClient.stop();
            Serial.println("MDNS lookup failed");
            return false;
        }
        Serial.print("Obtained host address: ");
        Serial.println(hostAddress);
        success = getNextSensorName(&httpClient,                    //
                                    hostAddress.toString().c_str(), //
                                    serverPort,                     //
                                    apiPath,                        //
                                    outSensorName,                  //
                                    bufferLength);
    }
    else
    {
        success = getNextSensorName(&httpClient,   //
                                    serverAddress, //
                                    serverPort,    //
                                    apiPath,       //
                                    outSensorName, //
                                    bufferLength);
    }
    httpClient.stop();
    return success;
}