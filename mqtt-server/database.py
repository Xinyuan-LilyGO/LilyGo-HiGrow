import sqlite3
import logging
import threading
import struct


class Database:

    __BYTES_DB_FORMAT_STRING = '-'
    __UTF8_DB_FORMAT_STRING = 'utf8'

    def __init__(self):
        self.__open = False
        self.__connection = None
        self.__cursor = None
        self.__db_lock = threading.Lock()

    def open(self, filename):
        return self.__open_connection_and_setup_schema(filename)

    def is_open(self):
        return self.__open

    def __open_connection_and_setup_schema(self, filename):
        try:
            with self.__db_lock:
                self.__connection = sqlite3.connect(
                    filename, check_same_thread=False)
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
                "data BLOB,"
                "format_string STRING"
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

    def get_data(self, topic, datetime_from=None, datetime_to=None):
        """
        Get all the data from the databaes for the given [topic] betwteen times
        [datetime_from] and [datetime_to].
        @param topic the topic string
        @param datetime_from the earliest time to get data from.  If None, returns data from (and including) the earliest recording of the database
        @param datetime_fo the latest time to get data from.  If None, returns data up until (and including) the most recent recording
        @returns a list of the data points
        """
        try:
            with self.__db_lock:
                time_column = "timestamp"
                sql = "SELECT `{}`, `data`, `format_string` FROM `events` WHERE `topic` == ? ORDER BY `{}` ASC".format(
                    time_column, time_column)
                self.__cursor.execute(sql, (topic,))
                data = self.__cursor.fetchall()
                if data is None or len(data) == 0:
                    return []

                # first column holds the datetime, second is the data (bytes), third is the format string
                data_decoded = []
                for d in data:
                    timestamp = d[0]
                    if d[2] == Database.__BYTES_DB_FORMAT_STRING:
                        data = d[1]
                    elif d[2] == Database.__UTF8_DB_FORMAT_STRING:
                        data = d[1].decode('utf-8')
                    else:
                        data = struct.unpack(d[2], d[1])[0]
                    data_decoded.append([timestamp, data])
                return data_decoded
        except Exception as e:
            logging.error(
                "Exception when trying to get topics list: {}".format(e))
            return []

    def write_message(self, topic, data):
        if isinstance(data, bytes) or isinstance(data, bytearray):
            data_bytes = data
            format_string = Database.__BYTES_DB_FORMAT_STRING
        elif isinstance(data, float):
            data_bytes = bytearray(struct.pack("f", data))
            format_string = 'f'
        elif isinstance(data, int):
            data_bytes = bytearray(struct.pack("i", data))
            format_string = 'i'
        elif isinstance(data, str):
            data_bytes = data.encode('utf-8')
            format_string = Database.__UTF8_DB_FORMAT_STRING
        else:
            raise Exception("data must be bytes, bytearray, float or int.")
        return self.__write_message(topic, data_bytes, format_string)

    def __write_message(self, topic, data: bytes, format_string: str):
        with self.__db_lock:
            try:
                # insert the message
                sql = "INSERT INTO `events` (`topic`, `data`, `format_string`) " \
                    "VALUES (?, ?, ?);"
                self.__cursor.execute(
                    sql, (topic, sqlite3.Binary(data), format_string,))
                self.__connection.commit()

            except Exception as e:
                logging.error(
                    "Exception when inserting row with topic {}: {}".format(topic, e))
                return False

            try:
                # insert the topic to keep a list of unique topics
                sql = "INSERT INTO 'topics' ('name') VALUES (?)"
                self.__cursor.execute(sql, (topic,))
                self.__connection.commit()

            except Exception as e:
                # this might not be an error, because if the topic already exists it will
                # not be reinserted since we require it to be unique
                logging.error(
                    "Exception when inserting row with topic {}: {}".format(topic, e))
            return True
