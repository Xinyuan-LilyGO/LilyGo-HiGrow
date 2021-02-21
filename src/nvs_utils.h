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

/// @brief Try to write the supplied sensor \p name into non volatile storage
/// @param[in] name The sensor name to try and store
/// @returns true if the save was successfull
bool writeSensorName(const char *name);

/// @brief Try to read the sensor name from non volatile storage (previously saved using writeSensorName)
/// @param name pointer to a buffer that is filled with the sensor name if successfull
/// @returns true if the read was successfull, in which case the buffer \p name is filled with the read name
bool tryReadSensorName(char *name);