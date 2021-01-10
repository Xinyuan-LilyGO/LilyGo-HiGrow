import server
import time
import database
import logging
import sys


def test_run_server():
    # setup the logger
    root = logging.getLogger()
    root.setLevel(logging.DEBUG)
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(logging.DEBUG)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    handler.setFormatter(formatter)
    root.addHandler(handler)

    s = server.Server(topic_filter="qdtest/number")
    s.initialise()
    time.sleep(5)
    s.uninitialise()

if __name__ == "__main__":
    test_run_server()
