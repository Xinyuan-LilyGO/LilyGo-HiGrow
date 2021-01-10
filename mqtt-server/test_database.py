import database
import time
import logging
import sys


def test_open_nonexistent_database():
    # setup the logger
    root = logging.getLogger()
    root.setLevel(logging.DEBUG)
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(logging.DEBUG)
    formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    handler.setFormatter(formatter)
    root.addHandler(handler)

    db = database.Database()
    assert db.open("thisdoesntexist.db")
    assert db.is_open()
    db.write_message("mytopic", "mydata".encode('utf-8'))
    db.close(wait_for_write=True)
    assert not db.is_open()


if __name__ == "__main__":
    test_open_nonexistent_database()
