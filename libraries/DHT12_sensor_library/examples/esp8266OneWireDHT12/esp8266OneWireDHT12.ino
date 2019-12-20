// Do not remove the include below
#include "Arduino.h"

#include <DHT12.h>

// Set dht12 pin to 5 and specify that is oneWire comunication (not default i2c)
DHT12 dht12(D5, true);

//The setup function is called once at startup of the sketch
void setup()
{
	Serial.begin(112560);
// Add your initialization code here
	dht12.begin();
}
int timeSinceLastRead = 0;

void loop()
{
	// Report every 2 seconds.
	if(timeSinceLastRead > 2000) {
		// Read temperature as Celsius (the default)
		float t12 = dht12.readTemperature();
		// Read temperature as Fahrenheit (isFahrenheit = true)
		float f12 = dht12.readTemperature(true);
		// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
		float h12 = dht12.readHumidity();

		bool dht12Read = true;
		// Check if any reads failed and exit early (to try again).
		if (isnan(h12) || isnan(t12) || isnan(f12)) {
		  Serial.println("Failed to read from DHT12 sensor!");

		  dht12Read = false;
		}

		if (dht12Read){
			// Compute heat index in Fahrenheit (the default)
			float hif12 = dht12.computeHeatIndex(f12, h12);
			// Compute heat index in Celsius (isFahreheit = false)
			float hic12 = dht12.computeHeatIndex(t12, h12, false);
			// Compute dew point in Fahrenheit (the default)
			float dpf12 = dht12.dewPoint(f12, h12);
			// Compute dew point in Celsius (isFahreheit = false)
			float dpc12 = dht12.dewPoint(t12, h12, false);


			Serial.print("DHT12=> Humidity: ");
			Serial.print(h12);
			Serial.print(" %\t");
			Serial.print("Temperature: ");
			Serial.print(t12);
			Serial.print(" *C ");
			Serial.print(f12);
			Serial.print(" *F\t");
			Serial.print("  Heat index: ");
			Serial.print(hic12);
			Serial.print(" *C ");
			Serial.print(hif12);
			Serial.print(" *F");
			Serial.print("  Dew point: ");
			Serial.print(dpc12);
			Serial.print(" *C ");
			Serial.print(dpf12);
			Serial.println(" *F");
		}
		timeSinceLastRead = 0;
	}
	delay(100);
	timeSinceLastRead += 100;
}
