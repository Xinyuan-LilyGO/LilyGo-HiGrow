/* DHT12 library

 MIT license
 written by Renzo Mischianti
 */

#include "DHT12.h"
#include "Wire.h"

// Default is i2c on default pin with default DHT12 adress
DHT12::DHT12(void) {

}

DHT12::DHT12(uint8_t addressOrPin, bool oneWire) {
	_isOneWire = oneWire;
	if (oneWire) {
		_pin = addressOrPin;
		#ifdef __AVR
			_bit = digitalPinToBitMask(_pin);
			_port = digitalPinToPort(_pin);
		#endif
		_maxcycles = microsecondsToClockCycles(1000); // 1 millisecond timeout for
													  // reading pulses from DHT sensor.
		// Note that count is now ignored as the DHT reading algorithm adjusts itself
		// basd on the speed of the processor.

		DEBUG_PRINTLN("PIN MODE");
	} else {
		_address = addressOrPin;
		DEBUG_PRINTLN("I2C MODE");
	}
}

// Is not good idea use other pin for i2c as standard on Arduino you can get lag.
// The lag happens when you choose "different pins", because you are then using a
// slow software emulation of the I2C hardware. The built in I2C hardware has fixed pin assignments.
#ifndef __AVR
	DHT12::DHT12(uint8_t sda, uint8_t scl) {
		_isOneWire = false;
		_sda = sda;
		_scl = scl;
	}

	DHT12::DHT12(uint8_t sda, uint8_t scl, uint8_t address) {
		_isOneWire = false;
		_sda = sda;
		_scl = scl;
		_address = address;
	}
#endif

void DHT12::begin() {
	_lastreadtime = -(MIN_ELAPSED_TIME + 1);
	if (_isOneWire) {
		// set up the pins!
		pinMode(_pin, INPUT_PULLUP);
		// Using this value makes sure that millis() - lastreadtime will be
		// >= MIN_INTERVAL right away. Note that this assignment wraps around,
		// but so will the subtraction.
		DEBUG_PRINT("Max clock cycles: ");
		DEBUG_PRINTLN(_maxcycles, DEC);
	} else {
		#ifndef __AVR
			Wire.begin(_sda, _scl);
		#else
//			Default pin for AVR some problem on software emulation
//			#define SCL_PIN _scl
// 			#define SDA_PIN _sda
			Wire.begin();
		#endif
		DEBUG_PRINT("I2C Inizialization: sda, scl: ");
		DEBUG_PRINT(_sda);
		DEBUG_PRINT(",");
		DEBUG_PRINTLN(_scl);
	}
}

DHT12::ReadStatus DHT12::readStatus(bool force) {
	// Check if sensor was read less than two seconds ago and return early
	// to use last reading.
	uint32_t currenttime = millis();
	if (!force && ((currenttime - _lastreadtime) < MIN_ELAPSED_TIME)) {
		return _lastresult; // return last correct measurement
	}
	_lastreadtime = currenttime;

	if (_isOneWire) {
		// Reset 40 bits of received data to zero.
		data[0] = data[1] = data[2] = data[3] = data[4] = 0;

		// Send start signal.  See DHT datasheet for full signal diagram:
		//   http://www.adafruit.com/datasheets/Digital%20humidity%20and%20temperature%20sensor%20AM2302.pdf

		  // Go into high impedence state to let pull-up raise data line level and
		  // start the reading process.
		  digitalWrite(_pin, HIGH);
		  delay(250);

		  // First set data line low for 20 milliseconds.
		  pinMode(_pin, OUTPUT);
		  digitalWrite(_pin, LOW);
		  delay(20);

		  uint32_t cycles[80];
		  {
		    // Turn off interrupts temporarily because the next sections are timing critical
		    // and we don't want any interruptions.
			  InterruptLockDht12 lock;

		    // End the start signal by setting data line high for 40 microseconds.
		    digitalWrite(_pin, HIGH);
		    delayMicroseconds(40);

		    // Now start reading the data line to get the value from the DHT sensor.
		    pinMode(_pin, INPUT_PULLUP);
		    delayMicroseconds(10);  // Delay a bit to let sensor pull data line low.

			// First expect a low signal for ~80 microseconds followed by a high signal
			// for ~80 microseconds again.
			if (expectPulse(LOW) == 0) {
				DEBUG_PRINTLN(F("Timeout waiting for start signal low pulse."));
				_lastresult = ERROR_TIMEOUT_LOW;
				return _lastresult;
			}
			if (expectPulse(HIGH) == 0) {
				DEBUG_PRINTLN(F("Timeout waiting for start signal high pulse."));
				_lastresult = ERROR_TIMEOUT_HIGH;
				return _lastresult;
			}

			// Now read the 40 bits sent by the sensor.  Each bit is sent as a 50
			// microsecond low pulse followed by a variable length high pulse.  If the
			// high pulse is ~28 microseconds then it's a 0 and if it's ~70 microseconds
			// then it's a 1.  We measure the cycle count of the initial 50us low pulse
			// and use that to compare to the cycle count of the high pulse to determine
			// if the bit is a 0 (high state cycle count < low state cycle count), or a
			// 1 (high state cycle count > low state cycle count). Note that for speed all
			// the pulses are read into a array and then examined in a later step.
			for (int i = 0; i < 80; i += 2) {
				cycles[i] = expectPulse(LOW);
				cycles[i + 1] = expectPulse(HIGH);
			}

			  // Inspect pulses and determine which ones are 0 (high state cycle count < low
			  // state cycle count), or 1 (high state cycle count > low state cycle count).
			  for (int i=0; i<40; ++i) {
			    uint32_t lowCycles  = cycles[2*i];
			    uint32_t highCycles = cycles[2*i+1];
			    if ((lowCycles == 0) || (highCycles == 0)) {
			      DEBUG_PRINTLN(F("Timeout waiting for pulse."));
			      _lastresult = ERROR_TIMEOUT;
			      return _lastresult;
			    }
			    data[i/8] <<= 1;
			    // Now compare the low and high cycle times to see if the bit is a 0 or 1.
			    if (highCycles > lowCycles) {
			      // High cycles are greater than 50us low cycle count, must be a 1.
			      data[i/8] |= 1;
			    }
			    // Else high cycles are less than (or equal to, a weird case) the 50us low
			    // cycle count so this must be a zero.  Nothing needs to be changed in the
			    // stored data.
			  }

			  DEBUG_PRINTLN(F("Received:"));
			  DEBUG_PRINT(data[0], HEX); DEBUG_PRINT(F(", "));
			  DEBUG_PRINT(data[1], HEX); DEBUG_PRINT(F(", "));
			  DEBUG_PRINT(data[2], HEX); DEBUG_PRINT(F(", "));
			  DEBUG_PRINT(data[3], HEX); DEBUG_PRINT(F(", "));
			  DEBUG_PRINT(data[4], HEX); DEBUG_PRINT(F(" =? "));
			  DEBUG_PRINTLN((data[0] + data[1] + data[2] + data[3]) & 0xFF, HEX);

				DHT12::ReadStatus cks = DHT12::_checksum();
				if (cks != OK) {
					DEBUG_PRINTLN("CHECKSUM ERROR!");
					_lastresult = cks;
					return cks;
				}

			_lastresult = OK;
			return OK;
		}
//		return DHT12::_readSensor(DHTLIB_DHT_WAKEUP, DHTLIB_DHT_LEADING_ZEROS);
//		return DHT12::_readSensor(DHTLIB_DHT11_WAKEUP, DHTLIB_DHT11_LEADING_ZEROS);

	} else {
		DEBUG_PRINT("I2C START READING..");
		Wire.beginTransmission(_address);
		Wire.write(0);
		if (Wire.endTransmission() != 0) {
			DEBUG_PRINTLN("CONNECTION ERROR!");
			_lastresult = ERROR_CONNECT;
			return _lastresult;
		}
		Wire.requestFrom(_address, (uint8_t) 5);
		for (uint8_t i = 0; i < 5; ++i) {
			data[i] = Wire.read();
			DEBUG_PRINTLN(data[i]);
		}

		delay(1);
		if (Wire.available() != 0) {
			DEBUG_PRINTLN("TIMEOUT ERROR!");
			_lastresult = ERROR_TIMEOUT;
			return _lastresult;
		}

		DHT12::ReadStatus cks = DHT12::_checksum();
		if (cks != OK) {
			DEBUG_PRINTLN("CHECKSUM ERROR!");
			_lastresult = cks;
			return cks;
		}

		DEBUG_PRINTLN("...READING OK");
		_lastresult = OK;
		return _lastresult;
	}
}

bool DHT12::read(bool force) {
	ReadStatus chk = DHT12::readStatus(force);
	DEBUG_PRINT(F("\nRead sensor: "));
	DEBUG_PRINT((chk != DHT12::OK));
	switch (chk) {
	case DHT12::OK:
		DEBUG_PRINTLN(F("OK"));
		break;
	case DHT12::ERROR_CHECKSUM:
		DEBUG_PRINTLN(F("Checksum error"))
		;
		break;
	case DHT12::ERROR_TIMEOUT:
		DEBUG_PRINTLN(F("Timeout error"))
		;
		break;
	case DHT12::ERROR_TIMEOUT_HIGH:
		DEBUG_PRINTLN(F("Timeout error high"))
		;
		break;
	case DHT12::ERROR_TIMEOUT_LOW:
		DEBUG_PRINTLN(F("Timeout error low"))
		;
		break;
	case DHT12::ERROR_CONNECT:
		DEBUG_PRINTLN(F("Connect error"))
		;
		break;
	case DHT12::ERROR_ACK_L:
		DEBUG_PRINTLN(F("AckL error"))
		;
		break;
	case DHT12::ERROR_ACK_H:
		DEBUG_PRINTLN(F("AckH error"))
		;
		break;
	case DHT12::ERROR_UNKNOWN:
		DEBUG_PRINTLN(F("Unknown error DETECTED"))
		;
		break;
	case DHT12::NONE:
		DEBUG_PRINTLN(F("No result"))
		;
		break;
	default:
		DEBUG_PRINTLN(F("Unknown error"))
		;
		break;
	}
	return (chk == DHT12::OK);
}

float DHT12::convertCtoF(float c) {
	return c * 1.8 + 32;
}

float DHT12::convertFtoC(float f) {
	return (f - 32) * 0.55555;
}

// boolean isFahrenheit: True == Fahrenheit; False == Celsius
float DHT12::computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit) {
	// Using both Rothfusz and Steadman's equations
	// http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
	float hi;

	if (!isFahrenheit)
		temperature = convertCtoF(temperature);

	hi = 0.5
			* (temperature + 61.0 + ((temperature - 68.0) * 1.2)
					+ (percentHumidity * 0.094));

	if (hi > 79) {
		hi = -42.379 + 2.04901523 * temperature + 10.14333127 * percentHumidity
				+ -0.22475541 * temperature * percentHumidity
				+ -0.00683783 * pow(temperature, 2)
				+ -0.05481717 * pow(percentHumidity, 2)
				+ 0.00122874 * pow(temperature, 2) * percentHumidity
				+ 0.00085282 * temperature * pow(percentHumidity, 2)
				+ -0.00000199 * pow(temperature, 2) * pow(percentHumidity, 2);

		if ((percentHumidity < 13) && (temperature >= 80.0)
				&& (temperature <= 112.0))
			hi -= ((13.0 - percentHumidity) * 0.25)
					* sqrt((17.0 - abs(temperature - 95.0)) * 0.05882);

		else if ((percentHumidity > 85.0) && (temperature >= 80.0)
				&& (temperature <= 87.0))
			hi += ((percentHumidity - 85.0) * 0.1)
					* ((87.0 - temperature) * 0.2);
	}

	return isFahrenheit ? hi : convertFtoC(hi);
}

float DHT12::readHumidity(bool force) {
	DEBUG_PRINTLN("----------------------------");
	float humidity = NAN;

	if (_isOneWire) {
		if (DHT12::read(force)) {
			DEBUG_PRINT(data[0]);
//			humidity = data[0];
			humidity = (data[0] + (float) data[1] / 10);
		}
	} else {
		if (DHT12::read(force)) {
			humidity = (data[0] + (float) data[1] / 10);
		}
	}
	return humidity;
}

// boolean S == Scale.  True == Fahrenheit; False == Celsius
float DHT12::readTemperature(bool scale, bool force) {
	float temperature = NAN;
	if (_isOneWire) {
		if (DHT12::read(force)) {
//			temperature = data[2];
//			if (scale) {
//				temperature = convertCtoF(temperature);
//			}
			byte scaleValue = data[3] & B01111111;
			byte signValue  = data[3] & B10000000;

			temperature = (data[2] + (float) scaleValue / 10);// ((data[2] & 0x7F)*256 + data[3]);
			    if (signValue)  // negative temperature
			        temperature = -temperature;

			if (scale) {
				temperature = convertCtoF(temperature);
			}
		}
	} else {

		bool r = DHT12::read(force);
		DEBUG_PRINT("READ ---> ");
		DEBUG_PRINTLN(r);
		if (r) {
			DEBUG_PRINT("BIT 0 -> ");
			DEBUG_PRINTLN(data[0], BIN);
			DEBUG_PRINT("BIT 1 -> ");
			DEBUG_PRINTLN(data[1], BIN);
			DEBUG_PRINT("BIT 2 -> ");
			DEBUG_PRINTLN(data[2], BIN);
			DEBUG_PRINT("BIT 3 -> ");
			DEBUG_PRINTLN(data[3], BIN);
			DEBUG_PRINT("BIT 4 -> ");
			DEBUG_PRINTLN(data[4], BIN);
			DEBUG_PRINT("BIT 5 -> ");
			DEBUG_PRINTLN(data[5], BIN);

			byte scaleValue = data[3] & B01111111;
			byte signValue  = data[3] & B10000000;

			temperature = (data[2] + (float) scaleValue / 10);// ((data[2] & 0x7F)*256 + data[3]);
			    if (signValue)  // negative temperature
			        temperature = -temperature;

			if (scale) {
				temperature = convertCtoF(temperature);
			}
		}
	}
	return temperature;

}

#include <math.h>
// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//
// boolean S == Scale.  True == Fahrenheit; False == Celsius
float DHT12::dewPoint(float temperature, float humidity, bool isFahrenheit) {
	// sloppy but good approximation for 0 ... +70 °C with max. deviation less than 0.25 °C

	float temp;

	if(!isFahrenheit){
		temp = temperature;
	} else {
		temp = convertFtoC(temperature);
	}

	float humi = humidity;
	float ans =  (temp - (14.55 + 0.114 * temp) * (1 - (0.01 * humi)) - pow(((2.5 + 0.007 * temp) * (1 - (0.01 * humi))),3) - (15.9 + 0.117 * temp) * pow((1 - (0.01 * humi)), 14));

	if(!isFahrenheit){
		return ans;           // returns dew Point in Celsius
	}

	return convertCtoF(ans);   // returns dew Point in Fahrenheit
}

//////// PRIVATE
DHT12::ReadStatus DHT12::_checksum() {
	uint8_t sum = data[0] + data[1] + data[2] + data[3];
	if (data[4] != sum)
		return ERROR_CHECKSUM;
	return OK;
}

// Expect the signal line to be at the specified level for a period of time and
// return a count of loop cycles spent at that level (this cycle count can be
// used to compare the relative time of two pulses).  If more than a millisecond
// ellapses without the level changing then the call fails with a 0 response.
// This is adapted from Arduino's pulseInLong function (which is only available
// in the very latest IDE versions):
//   https://github.com/arduino/Arduino/blob/master/hardware/arduino/avr/cores/arduino/wiring_pulse.c
uint32_t DHT12::expectPulse(bool level) {
	uint32_t count = 0;
	// On AVR platforms use direct GPIO port access as it's much faster and better
	// for catching pulses that are 10's of microseconds in length:
#ifdef __AVR
	uint8_t portState = level ? _bit : 0;
	while ((*portInputRegister(_port) & _bit) == portState) {
		if (count++ >= _maxcycles) {
			return 0; // Exceeded timeout, fail.
		}
	}
	// Otherwise fall back to using digitalRead (this seems to be necessary on ESP8266
	// right now, perhaps bugs in direct port access functions?).
#else
	while (digitalRead(_pin) == level) {
		if (count++ >= _maxcycles) {
			return 0; // Exceeded timeout, fail.
		}
	}
#endif

	return count;
}
