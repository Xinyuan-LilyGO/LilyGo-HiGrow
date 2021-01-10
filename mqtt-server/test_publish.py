import paho.mqtt.client as mqtt
import time


def test_publish():
    MQTT_HOST = "test.mosquitto.org"
    MQTT_HOST_PORT = 1883

    client = mqtt.Client("Publisher")

    def on_log(client, userdata, level, buf):
        print("log: ",buf)

    client.on_log = on_log
    client.connect(MQTT_HOST, port=MQTT_HOST_PORT)

    for i in range(10):
        client.publish("qdtest/number", 1.5)
        time.sleep(0.5)

    assert True

if __name__ == "__main__":
    test_publish()