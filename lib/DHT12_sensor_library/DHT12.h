/** \mainpage DHT12 sensor library
 *
 * MIT license
 * written by Renzo Mischianti
 */

#ifndef DHT12_h
#define DHT12_h

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "Wire.h"

#define DEFAULT_DHT12_ADDRESS 0x5C;
#define DEFAULT_SDA SDA;
#define DEFAULT_SCL SCL;

#define MIN_ELAPSED_TIME 2000

// Uncomment to enable printing out nice debug messages.
//#define DHT_DEBUG

// Define where debug output will be printed.
#define DEBUG_PRINTER Serial

// Setup debug printing macros.
#ifdef DHT_DEBUG
	#define DEBUG_PRINT(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
	#define DEBUG_PRINTLN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#else
	#define DEBUG_PRINT(...) {}
	#define DEBUG_PRINTLN(...) {}
#endif

#ifndef __AVR
	#define DHTLIB_TIMEOUT 10000  // should be approx. clock/40000
#else
	#ifndef F_CPU
		#define DHTLIB_TIMEOUT 1000  // should be approx. clock/40000
	#else
		#define DHTLIB_TIMEOUT (F_CPU/40000)
	#endif
#endif

class DHT12 {
public:
	/**Status of the read*/
	enum ReadStatus {
		OK, /**All ok*/
		ERROR_CHECKSUM, /**Error on checksum*/
		ERROR_TIMEOUT, /**Timeout error*/
		ERROR_TIMEOUT_LOW, /**Timeout on wait after low pulse*/
		ERROR_TIMEOUT_HIGH,/**Timeout on wait after high pulse*/
		ERROR_CONNECT, /**Connection error (Wire)*/
		ERROR_ACK_L,/**Acknowledge Low */
		ERROR_ACK_H,/**Acknowledge High */
		ERROR_UNKNOWN, /**Error unknown */
		NONE
	};

	/**
	 * Standard constructor, default wire connection on default SDA SCL pin
	 */
	DHT12(void);
	/**
	 * Constructor
	 * @param addressORPin If oneWire == true this is pin number if oneWire false this is address of i2c
	 * @param oneWire select if is oneWire of i2c
	 */
	DHT12(uint8_t addressORPin, bool oneWire = false);
#ifndef __AVR
	/**
	 * Additional parameter non tested for Arduino, Arduino very slow on software i2c
	 */
	DHT12(uint8_t sda, uint8_t scl);
	DHT12(uint8_t sda, uint8_t scl, uint8_t address);
#endif
	/**
	 * Start handshake
	 */
	void begin(void);
	/**
	 * Read temperature
	 * @param scale Select false --> Celsius true --> Fahrenheit
	 * @param force
	 * @return
	 */
	float readTemperature(bool scale = false, bool force = false);
	/**
	 * Convert Celsius to Fahrenheit
	 * @param
	 * @return
	 */
	float convertCtoF(float);
	/**
	 * Convert Fahrenheit to Celsius
	 * @param
	 * @return
	 */
	float convertFtoC(float);
	/**
	 * The heat index (HI) or humiture is an index that combines air temperature and relative humidity, in shaded areas, as an attempt to determine the human-perceived equivalent temperature, as how hot it would feel if the humidity were some other value in the shade. The result is also known as the "felt air temperature" or "apparent temperature".
	 * @param temperature
	 * @param percentHumidity
	 * @param isFahrenheit specify scale of temperature
	 * @return
	 */
	float computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit = true);
	/**
	 * Read humidity percentage
	 * @param force Force to request new data (if 2secs is not passed from previous request)
	 * @return
	 */
	float readHumidity(bool force = false);
	/**
	 * Dew point the atmospheric temperature (varying according to pressure and humidity) below which water droplets begin to condense and dew can form.
	 * @param temperature
	 * @param humidity
	 * @param isFahrenheit specify scale of temperature
	 * @return
	 */
	float dewPoint(float temperature, float humidity, bool isFahrenheit = true);
	/**
	 * Read and return status of the read
	 * @param force
	 * @return
	 */
	ReadStatus readStatus(bool force = false);bool read(bool force = false);

private:
	bool _isOneWire = false;

	uint8_t data[5];
	uint8_t _address = DEFAULT_DHT12_ADDRESS
	;
	uint8_t _sda = DEFAULT_SDA
	;
	uint8_t _scl = DEFAULT_SCL
	;

	uint32_t _lastreadtime = 0;
	ReadStatus _lastresult = NONE;

	uint8_t _pin = 3;
#ifdef __AVR
	// Use direct GPIO access on an 8-bit AVR so keep track of the port and bitmask
	// for the digital pin connected to the DHT.  Other platforms will use digitalRead.
	uint8_t _bit = 0, _port = 0;
#endif
	uint32_t _maxcycles = 0;

	ReadStatus _checksum(void);
	uint32_t expectPulse(bool level);
	ReadStatus _readSensor(uint8_t wakeupDelay, uint8_t leadingZeroBits);

};

class InterruptLockDht12 {
public:
	InterruptLockDht12() {
		noInterrupts();
	}
	~InterruptLockDht12() {
		interrupts();
	}

};

#endif
