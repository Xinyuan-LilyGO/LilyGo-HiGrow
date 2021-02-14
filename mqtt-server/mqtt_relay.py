import paho.mqtt.client as mqtt
import logging
import threading


class MQTTRelay:

    def __init__(self, topic_filter='#',
                 mqtt_host="test.mosquitto.org",
                 mqt_host_port=1883,
                 enable_logging=True):
        """
        optionally set a topic filter as [topic_filter]
        by default subscribe to all messages ('#'), not recommended (https://www.hivemq.com/blog/mqtt-essentials-part-5-mqtt-topics-best-practices/)
        """
        super(MQTTRelay, self).__init__()
        
        self.__client = None
        # thread that runs the "loop_forever" method of the mqtt client
        self.__loop_thread = None
        self.__topic_filter = topic_filter
        self.__mqtt_host = mqtt_host
        self.__mqt_host_port = mqt_host_port
        self.__enable_logging = enable_logging

        self.__new_topic_callbacks = []
        self.__new_data_callbacks = []

        self.__observed_topics = set()

    def get_observed_topics(self):
        self.__observed_topics

    def initialise(self):

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

    def register_new_data_callback(self, callback):
        self.__new_data_callbacks.append(callback)

    def register_new_topic_callback(self, callback):
        self.__new_topic_callbacks.append(callback)

    def __on_message(self, client, user_data, message):
        logging.info("Message recieved. topic={}, qos={}, retain={}, length={}".format(
                     message.topic, message.qos, message.retain, len(message.payload)))

        init_num_topics = len(self.__observed_topics)
        self.__observed_topics.add(message.topic)

        # if the number of observed topics just increased, notify that we got a new one
        if len(self.__observed_topics) > init_num_topics:
            for new_topic_callback in self.__new_topic_callbacks:
                threading.Thread(target=new_topic_callback, args=(message.topic,)).start()

        for new_data_callback in self.__new_data_callbacks:
            threading.Thread(target=new_data_callback, args=(message.topic, message.payload,)).start()
