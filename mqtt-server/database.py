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
        self.__db_lock = threading.Lock()

    def open(self, filename, threaded=False):
        return self.__open_connection_and_setup_schema(filename)

    def is_open(self):
        return self.__open

    def __open_connection_and_setup_schema(self, filename):
        try:
            with self.__db_lock:
                self.__connection = sqlite3.connect(filename, check_same_thread=False)
                self.__cursor = self.__connection.cursor()
                self.__open = True
        except Exception as e:
            logging.error(
                "Exception when opening database from '{}'".format(filename))
            return False

        self.__setup_schema()
        return True

    def __setup_schema(self):
        if not self.__open:
            raise Exception("Database wasn't open")

        with self.__db_lock:
            # create the events table
            # (a table of all observed messages, ever)
            self.__cursor.execute(
                "CREATE TABLE IF NOT EXISTS events("
                "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                "topic TEXT NOT NULL,"
                "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL,"
                "data BLOB"
                ");")
            self.__connection.commit()

            # create the topics table
            # (a table of all observed topics, ever)
            self.__cursor.execute(
                "CREATE TABLE IF NOT EXISTS topics("
                "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                "name TEXT UNIQUE NOT NULL"
                ");")
            self.__connection.commit()

    def close(self, wait_for_write=False):
        """
        Close the database
        """
        self.__close()

    def __close(self):
        """
        Close the database file
        """
        with self.__db_lock:
            if self.__open:
                self.__connection.close()
                self.__open = False

        return self.__open

    def get_topics(self):
        """
        Return a list of the topics in the database
        """
        try:
            with self.__db_lock:
                sql = "SELECT * FROM 'topics' ORDER BY 'name' ASC"
                self.__cursor.execute(sql)
                topics = self.__cursor.fetchall()
                if topics is None or len(topics) == 0:
                    return []
                return [topic[1] for topic in topics]
        except Exception as e:
            logging.error(
                "Exception when trying to get topics list: {}".format(e))
            return []

    def write_message(self, topic, data: bytes):
        return self.__write_message(topic, data)

    def __write_message(self, topic, data: bytes):
        try:
            with self.__db_lock:
                # insert the message
                sql = "INSERT INTO 'events' (topic, data) " \
                    "VALUES (?, ?);"
                self.__cursor.execute(sql, (topic, sqlite3.Binary(data),))
                self.__connection.commit()

                # insert the topic to keep a list of unique topics
                sql = "INSERT INTO 'topics' ('name') VALUES (?)"
                self.__cursor.execute(sql, (topic,))
                self.__connection.commit()

            return True
        except Exception as e:
            logging.error(
                "Exception when inserting row with topic {}: {}".format(topic, e))
            return False
