import mqtt_relay
import time
import database
import logging
import sys


def new_topic_callback(topic):
    print("New topic: {}".format(topic))


def new_data_callback(topic, data):
    print("New data: {}".format(topic, data))


def test_run_server():
    # setup the logger
    root = logging.getLogger()
    root.setLevel(logging.DEBUG)
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(logging.DEBUG)
    formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    handler.setFormatter(formatter)
    root.addHandler(handler)

    s = mqtt_relay.MQTTRelay(topic_filter="otsensor/+")
    s.new_data.connect(new_data_callback)
    s.new_topic.connect(new_topic_callback)
    s.initialise()
    time.sleep(50)
    s.uninitialise()


if __name__ == "__main__":
    test_run_server()
