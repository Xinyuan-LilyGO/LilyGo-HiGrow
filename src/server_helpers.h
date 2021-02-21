#ifndef __SERVER_HELPERS__
#define __SERVER_HELPERS__

#include <WiFi.h>

/// @brief try to get a valid sensor name from the server
/// @param serverAddress The server address
/// @param serverPort The port to connect to on the server
/// @param apiPath the path that is GET queried to retrieve the sensor name (GET serverAddress/apiPath)
/// @param outSensorName preassigned buffer where the sensor name is put
/// @param bufferLength the size of the buffer \p outSensorName
/// @param mdnsLookup If this is true, then \p serverAddress is assumed to be a local address (without the .local), and the physical address is found by MDNS lookup.
bool getNextSensorName(WiFiClient *wifiClient,    //
                       const char *serverAddress, //
                       uint16_t serverPort,       //
                       const char *apiPath,       //
                       char *outSensorName,       //
                       uint8_t bufferLength,      //
                       bool mdnsLookup);

#endif