import paho.mqtt.client as mqtt
import time


MQTT_HOST = "test.mosquitto.org"
MQTT_HOST_PORT = 1883

client = mqtt.Client("Listener")

def on_message(client, user_data, message):
    print("message received " ,str(message.payload.decode("utf-8")))
    print("message topic=",message.topic)
    print("message qos=",message.qos)
    print("message retain flag=",message.retain)

def on_log(client, userdata, level, buf):
    print("log: ",buf)

client.on_message = on_message
client.on_log = on_log

client.connect(MQTT_HOST, port=MQTT_HOST_PORT)
client.subscribe("temperature")
client.loop_start()
while True:
    time.sleep(1)