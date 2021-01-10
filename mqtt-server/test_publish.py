import paho.mqtt.client as mqtt
import time


MQTT_HOST = "test.mosquitto.org"
MQTT_HOST_PORT = 1883

client = mqtt.Client("Publisher")

def on_log(client, userdata, level, buf):
    print("log: ",buf)

client.on_log = on_log
client.connect(MQTT_HOST, port=MQTT_HOST_PORT)

while True:
    client.publish("temperature", 1.5)
    time.sleep(1)