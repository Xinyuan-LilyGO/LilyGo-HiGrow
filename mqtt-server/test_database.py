import database
import time
import logging
import sys


# setup the logger
root = logging.getLogger()
root.setLevel(logging.DEBUG)
handler = logging.StreamHandler(sys.stdout)
handler.setLevel(logging.DEBUG)
formatter = logging.Formatter(
    '%(asctime)s - %(name)s - %(levelname)s - %(message)s')
handler.setFormatter(formatter)
root.addHandler(handler)


def test_write_to_database_sync():

    db = database.Database()
    assert db.open("thisdoesntexist.db", threaded=False)
    assert db.is_open()
    db.write_message("mytopic_sync", "mydata_sync".encode('utf-8'))
    db.close()
    assert not db.is_open()


def test_write_to_database_async():
    db = database.Database()
    assert db.open("thisdoesntexist.db", threaded=True)
    assert db.is_open()
    db.write_message("mytopic_async", "mydata_async".encode('utf-8'))
    db.close(wait_for_write=True)
    assert not db.is_open()


if __name__ == "__main__":
    test_write_to_database_sync()
    test_write_to_database_async()
