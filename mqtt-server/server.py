from flask import Flask
from flask import render_template, jsonify, request
from bokeh.models.sources import AjaxDataSource
from bokeh.plotting import figure
from bokeh.embed import components
from mqtt_relay import MQTTRelay
from PyQt5.QtWidgets import QApplication
import threading
import os
import logging
import database


topic_data = {}


def new_topic_callback(topic):
    if topic not in topic_data:
        topic_data[topic] = [[], []]


def new_data_callback(topic, data):
    print("New data: {} on topic {}\n".format(data, topic))
    # if topic in topic_data:
    #     if len(topic_data[topic][0]) == 0:
    #         t = 0
    #         topic_data[topic][0].append(t)
    #         topic_data[topic][1].append(float(data))


app = Flask(__name__)
qtapp = QApplication([])

if __name__ == "__main__":
    # make database instance
    database_path = "databases"
    database_name = "database.db"
    if not os.path.exists(database_path):
        logging.info('Creating directory "{}"'.format(database_path))
        os.mkdir(database_path)

    db_path = os.path.join(database_path, database_name)
    logging.info("Connecting to database '{}'".format(db_path))
    database = database.Database()
    database.open(db_path)

    relay = MQTTRelay(topic_filter="otsensor/+")
    relay.new_data.connect(new_data_callback)
    relay.new_topic.connect(new_topic_callback)


@app.route('/data/<topic>', methods=['POST'])
def data():
    if topic in topic_data:
        data = topic_data[topic]
        return jsonify(x=data[0], y=data[1])

    return jsonify(x=[], y=[])


@app.route('/dashboard/')
def show_dashboard():
    plots = {}
    for topic in relay.get_observed_topics():
        plots[topic] = make_ajax_plot()

    dash = render_template('dashboard.html', plots=plots)
    return dash


def make_ajax_plot(topic):
    source = AjaxDataSource(data_url=request.url_root + 'data/{}'.format(topic),
                            polling_interval=2000, mode='replace')

    source.data = dict(x=[], y=[])

    plot = figure(plot_height=300, sizing_mode='scale_width', title=topic)
    plot.line('x', 'y', source=source, line_width=4)

    script, div = components(plot)
    return script, div


def make_plot():
    plot = figure(plot_height=300, sizing_mode='scale_width')
    x = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    y = [2**v for v in x]
    plot.line(x, y, line_width=4)
    script, div = components(plot)
    return script, div


if __name__ == "__main__":

    relay.initialise()
    threading.Thread(target=app.run, kwargs={
                     'port': 80, 'debug': False, 'use_reloader': False}).start()
    print("Flask server started...")

    # we need to run the Qt application in order to have Qt signals work properly
    print("Starting qt app...")
    qtapp.exec_()

    # close things
    relay.uninitialise()
    database.close()
