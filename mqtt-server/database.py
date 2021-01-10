import sqlite3
import logging
import threading
import queue
import time


class Database:

    def __init__(self):
        self.__open = False
        self.__connection = None
        self.__cursor = None
        self.__message_queue = queue.Queue()
        self.__kill_db_thread = False
        self.__db_thread = None  # the thread that does database operations
        # 0 - idle, 1 - starting, 2 - started (in loop), 3 - shuttingdown, 4 - finished
        self.__db_thread_state = 0
        self.__db_thread_start_condition = threading.Condition()

    def open(self, filename):
        self.__kill_db_thread = False
        self.__db_thread = threading.Thread(
            target=self.__open_and_loop, args=(filename,))
        self.__db_thread.start()
        with self.__db_thread_start_condition:
            self.__db_thread_start_condition.wait_for(
                self.__db_thread_started_or_finished, timeout=5)
        return self.__open

    def is_open(self):
        return self.__open

    def __db_thread_started_or_finished(self):
        return self.__db_thread_state == 2 or self.__db_thread_state == 4

    def __notify_loop_state(self, new_state):
        self.__db_thread_state = new_state
        with self.__db_thread_start_condition:
            self.__db_thread_start_condition.notify_all()

    def __open_and_loop(self, filename):
        """
        Open the database from file
        If the file doesn't exist, it will be created
        """

        self.__db_thread_state = 1  # starting up
        try:
            self.__connection = sqlite3.connect(filename)
            self.__cursor = self.__connection.cursor()
            self.__open = True
        except Exception as e:
            logging.error(
                "Exception when opening database from '{}'".format(filename))
            self.__open = False
        finally:
            self.__notify_loop_state(4)

        self.__setup_schema()

        self.__notify_loop_state(2)

        while not self.__kill_db_thread:
            logging.debug("DB loop waiting for message...")
            item = self.__message_queue.get(True)
            if item is None:
                logging.debug("DB loop shutting down")
                break
            else:
                logging.debug("DB loop got message")
                assert isinstance(item, tuple)
                assert len(item) >= 2
                topic = item[0]
                data = item[1]
                self.__write_message(topic, data)

        self.__notify_loop_state(3)
        logging.debug("DB closing connection...")
        self.__close()
        logging.debug("DB connection closed")
        self.__notify_loop_state(4)

    def __setup_schema(self):
        if not self.__open:
            raise Exception("Database wasn't open")

        self.__cursor.execute(
            "CREATE TABLE IF NOT EXISTS events("
            "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
            "topic TEXT NOT NULL,"
            "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL,"
            "data BLOB"
            ");")
        self.__connection.commit()

    def close(self, wait_for_write=False):
        """
        Close the database
        if [wait_for_write] is True, wait for any pending messages to be written to the database before closing
        """

        # if we want to halt immediately (after the current message is done being process)
        # set the flag so that after the current message is handled, the loop will die
        if not wait_for_write:
            self.__kill_db_thread = True
        # wake the queue up with a None message (that will end the loop it if it gets to it)
        self.__message_queue.put(None)
        self.__db_thread.join()
        self.__db_thread = None

    def __close(self):
        """
        Close the database file
        """
        if self.__open:
            self.__connection.close()
            self.__open = False

        return self.__open

    def write_message(self, topic, data: bytes):
        self.__message_queue.put((topic, data))

    def __write_message(self, topic, data: bytes):
        try:
            sql = "INSERT INTO 'events' " \
                "(topic, data) " \
                "VALUES (?, ?);"
            self.__cursor.execute(sql, (topic, sqlite3.Binary(data),))
            self.__connection.commit()
            return False
        except Exception as e:
            logging.error(
                "Exception when inserting row with topic {}: {}".format(topic, e))
            return True
