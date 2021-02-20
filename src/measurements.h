#ifndef __MEASUREMENTS__
#define __MEASUREMENTS__

#include "Arduino.h"

#include <BH1750.h>
#include "DHT12_sensor_library/DHT12.h"
#include "protos/measurements.pb.h"
#include "time_helpers.h"

bool takeMeasurements(BH1750 *lightMeter, DHT12 *dht12, ttgo_proto_Measurements *outMeasurements);

void printMeasurements(Print &printer, const ttgo_proto_Measurements &measurements);

#endif