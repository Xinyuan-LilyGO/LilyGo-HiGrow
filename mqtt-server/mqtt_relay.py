import paho.mqtt.client as mqtt
import time
import database
import logging
import os
import threading


class MQTTRelay:

    def __init__(self, topic_filter='#',
                 database_path="databases",
                 database_name="database.db",
                 mqtt_host="test.mosquitto.org",
                 mqt_host_port=1883,
                 enable_logging=True):
        """
        optionally set a topic filter as [topic_filter]
        by default subscribe to all messages ('#'), not recommended (https://www.hivemq.com/blog/mqtt-essentials-part-5-mqtt-topics-best-practices/)
        """
        self.__database = None
        self.__client = None
        # thread that runs the "loop_forever" method of the mqtt client
        self.__loop_thread = None
        self.__topic_filter = topic_filter
        self.__database_path = database_path
        self.__database_name = database_name
        self.__mqtt_host = mqtt_host
        self.__mqt_host_port = mqt_host_port
        self.__enable_logging = enable_logging

    def initialise(self):

        # connect to database
        if not os.path.exists(self.__database_path):
            logging.info('Creating directory "{}"'.format(
                self.__database_path))
            os.mkdir(self.__database_path)
        db_path = os.path.join(self.__database_path, self.__database_name)
        logging.info("Connecting to database '{}'".format(db_path))
        self.__database = database.Database()
        self.__database.open(db_path)

        # connect to MQTT broker and subscribe to all messages
        self.__client = mqtt.Client("Server")
        self.__client.connect(self.__mqtt_host, port=self.__mqt_host_port)
        self.__client.reconnect_delay_set()
        self.__client.on_message = self.__on_message
        if self.__enable_logging:
            self.__client.enable_logger()

        self.__client.subscribe(self.__topic_filter)
        self.__loop_thread = threading.Thread(
            target=self.__client.loop_forever)
        self.__loop_thread.start()
        return True

    def uninitialise(self):
        logging.info("Disconnecting MQTT client")
        self.__client.disconnect()
        if self.__loop_thread is not None:
            self.__loop_thread.join()
            self.__loop_thread = None
        logging.info("MQTT thread stopped")
        self.__database.close()

    def __on_message(self, client, user_data, message):
        logging.info("Message recieved. topic={}, qos={}, retain={}, length={}".format(
                     message.topic, message.qos, message.retain, len(message.payload)))
        self.__database.write_message(message.topic, message.payload)
