#include "Arduino.h"

#define MAX_SSID_LENGTH 20
#define MAX_PWD_LENGTH 30
#define MAX_SENSOR_NAME 20

/// @brief initialise the NVS system
bool initNVS();

/// @param[out] ssid should be an char array of MAX_SSID_LENGTH elements
/// @param[out] pwd should be an char array of MAX_PWD_LENGTH elements
/// returns true if values were loaded from memory, false otherwise
bool tryReadSSIDPW(char *ssid, char *pwd);

/// @param[in] ssid should be an char array of maximum MAX_SSID_LENGTH elements.  If null, then the key is erased.
/// @param[in] pwd should be an char array of maximum MAX_PWD_LENGTH elements.  If null, then the key is erased.
/// returns true if values were written to memory, false otherwise
bool writeSSIDPW(const char *ssid, const char *pwd);

bool writeSensorName(const char *name);
bool tryReadSensorName(char *name);