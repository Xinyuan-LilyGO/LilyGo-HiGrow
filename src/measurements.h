#include "Arduino.h"

#include <BH1750.h>
#include "DHT12_sensor_library/DHT12.h"
#include "protos/measurements.pb.h"

bool takeMeasurements(BH1750 *lightMeter, DHT12 *dht12, ttgo_proto_Measurements *outMeasurements);

void printMeasurements(Print &printer, const ttgo_proto_Measurements &measurements);