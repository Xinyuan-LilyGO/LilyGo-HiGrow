Additional information and document update here on my site: [DHT12 Article](https://www.mischianti.org/2019/01/01/dht12-library-en/).

This is an Arduino and esp8266 library for the DHT12 series of very low cost temperature/humidity sensors (less than 1$) that work with i2c or one wire connection.

AI read that sometime seems that need calibration, but I have tree of this and get value very similar to DHT22. If you have calibration this problem, open issue on github and I add implementation.

Tutorial: 

To download. click the DOWNLOADS button in the top right corner, rename the uncompressed folder DHT12. Check that the DHT folder contains `DHT12.cpp` and `DHT12.h`. Place the DHT library folder your `<arduinosketchfolder>/libraries/` folder. You may need to create the libraries subfolder if its your first library. Restart the IDE.

# Reef complete DHT12 Humidity & Temperature

This libray try to emulate the behaivor of standard *DHT library sensors* (and copy a lot of code), and I add the code to manage i2c olso in the same manner.

The method is the same of *DHT library sensor*, with some adding like *dew point* function.

To use with i2c (default address and default SDA SCL pin) the constructor is:
```cpp
DHT12 dht12;
```
and take the default value for SDA SCL pin. (It's possible to redefine with specified contructor for esp8266, needed for ESP-01).
or
```cpp
DHT12 dht12(uint8_t addressOrPin)
```
`addressOrPin -> address`
to change address.

To use one wire:
```cpp
DHT12 dht12(uint8_t addressOrPin, true)
```
`addressOrPin -> pin`
boolean value is the selection of oneWire or i2c mode.

You can use It with "implicit", "simple read" or "fullread":
**Implicit**, *only the first read doing a true read of the sensor, the other read that become in 2secs. interval are the stored value of first read*.
```cpp
		// The read of sensor have 2secs of elapsed time, unless you pass force parameter
		// Read temperature as Celsius (the default)
		float t12 = dht12.readTemperature();
		// Read temperature as Fahrenheit (isFahrenheit = true)
		float f12 = dht12.readTemperature(true);
		// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
		float h12 = dht12.readHumidity();
		
		
		// Compute heat index in Fahrenheit (the default)
		float hif12 = dht12.computeHeatIndex(f12, h12);
		// Compute heat index in Celsius (isFahreheit = false)
		float hic12 = dht12.computeHeatIndex(t12, h12, false);
		// Compute dew point in Fahrenheit (the default)
		float dpf12 = dht12.dewPoint(f12, h12);
		// Compute dew point in Celsius (isFahreheit = false)
		float dpc12 = dht12.dewPoint(t12, h12, false);

```
**Simple read** to get a status of read.
```cpp
		// The read of sensor have 2secs of elapsed time, unless you pass force parameter
		bool chk = dht12.read(); // true read is ok, false read problem

		// Read temperature as Celsius (the default)
		float t12 = dht12.readTemperature();
		// Read temperature as Fahrenheit (isFahrenheit = true)
		float f12 = dht12.readTemperature(true);
		// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
		float h12 = dht12.readHumidity();
		
		// Compute heat index in Fahrenheit (the default)
		float hif12 = dht12.computeHeatIndex(f12, h12);
		// Compute heat index in Celsius (isFahreheit = false)
		float hic12 = dht12.computeHeatIndex(t12, h12, false);
		// Compute dew point in Fahrenheit (the default)
		float dpf12 = dht12.dewPoint(f12, h12);
		// Compute dew point in Celsius (isFahreheit = false)
		float dpc12 = dht12.dewPoint(t12, h12, false);

```
**Full read** to get a specified status.
```cpp
		// The read of sensor have 2secs of elapsed time, unless you pass force parameter
		DHT12::ReadStatus chk = dht12.readStatus();
		Serial.print(F("\nRead sensor: "));
		switch (chk) {
		case DHT12::OK:
			Serial.println(F("OK"));
			break;
		case DHT12::ERROR_CHECKSUM:
			Serial.println(F("Checksum error"));
			break;
		case DHT12::ERROR_TIMEOUT:
			Serial.println(F("Timeout error"));
			break;
		case DHT12::ERROR_TIMEOUT_LOW:
			Serial.println(F("Timeout error on low signal, try put high pullup resistance"));
			break;
		case DHT12::ERROR_TIMEOUT_HIGH:
			Serial.println(F("Timeout error on low signal, try put low pullup resistance"));
			break;
		case DHT12::ERROR_CONNECT:
			Serial.println(F("Connect error"));
			break;
		case DHT12::ERROR_ACK_L:
			Serial.println(F("AckL error"));
			break;
		case DHT12::ERROR_ACK_H:
			Serial.println(F("AckH error"));
			break;
		case DHT12::ERROR_UNKNOWN:
			Serial.println(F("Unknown error DETECTED"));
			break;
		case DHT12::NONE:
			Serial.println(F("No result"));
			break;
		default:
			Serial.println(F("Unknown error"));
			break;
		}

		// Read temperature as Celsius (the default)
		float t12 = dht12.readTemperature();
		// Read temperature as Fahrenheit (isFahrenheit = true)
		float f12 = dht12.readTemperature(true);
		// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
		float h12 = dht12.readHumidity();
		
		// Compute heat index in Fahrenheit (the default)
		float hif12 = dht12.computeHeatIndex(f12, h12);
		// Compute heat index in Celsius (isFahreheit = false)
		float hic12 = dht12.computeHeatIndex(t12, h12, false);
		// Compute dew point in Fahrenheit (the default)
		float dpf12 = dht12.dewPoint(f12, h12);
		// Compute dew point in Celsius (isFahreheit = false)
		float dpc12 = dht12.dewPoint(t12, h12, false);
	
```

With examples, there are the connection diagram, it's important to use correct pullup resistor.

Thanks to Bobadas, dplasa and adafruit, to share the code in github (where I take some code and ideas).

## DHT12 PIN ##

![DHT12 Pin](https://github.com/xreef/DHT12_sensor_library/blob/master/resources/DHT12_pinout.png) 

## DHT12 connection schema ##
ArduinoUNO i2c

![ArduinoUNO i2c](https://github.com/xreef/DHT12_sensor_library/blob/master/examples/ArduinoI2CDHT12/ArduinoI2CDHT12.png)

ArduinoUNO oneWire 

![ArduinoUNO oneWire](https://github.com/xreef/DHT12_sensor_library/blob/master/examples/ArduinoOneWireDHT12/ArduinoOneWireDHT12.png)

esp8266 (D1Mini) i2c

![esp8266 (D1Mini) i2c](https://github.com/xreef/DHT12_sensor_library/blob/master/examples/esp8266I2CDHT12/esp8266I2CDHT12.png)

esp8266 (D1Mini) oneWire

![esp8266 (D1Mini) oneWire](https://github.com/xreef/DHT12_sensor_library/blob/master/examples/esp8266OneWireDHT12/esp8266OneWireDHT12.png)
