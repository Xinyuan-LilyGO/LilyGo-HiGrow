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
database = database.Database()


def new_topic_callback(topic):
    if topic not in topic_data:
        topic_data[topic] = [[], []]


def new_data_callback(topic, data):
    # for now, all data is sent as formatted strings
    data_str = data.decode('utf-8')
    data_float = float(data_str)
    database.write_message(topic, data_float)
    print("New data: {} on topic {}".format(data_float, topic))
    if topic in topic_data:
        if len(topic_data[topic][0]) == 0:
            t = 0
        else:
            t = topic_data[topic][0][-1] + 1
        topic_data[topic][0].append(t)
        topic_data[topic][1].append(float(data))


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
    database.open(db_path)

    # first off, get all existing data
    topics = database.get_topics()
    for topic in topics:
        data = database.get_data(topic)
        the_data = [[], []]
        t = 0
        for d in data:
            the_data[0].append(t)
            the_data[1].append(d[1])
            t += 1
        topic_data[topic] = the_data

    relay = MQTTRelay(topic_filter="otsensor/+")
    relay.new_data.connect(new_data_callback)
    relay.new_topic.connect(new_topic_callback)


@app.route('/topics/')
def get_topics():
    return jsonify(database.get_topics())


@app.route('/data/<topic_root>/<topic_sub>/', methods=['POST'])
def get_data(topic_root, topic_sub):
    global topic_data
    topic = topic_root + "/" + topic_sub
    if topic in topic_data:
        data = topic_data[topic]
        return jsonify(x=data[0], y=data[1])

    return jsonify(x=[], y=[])


@app.route('/dashboard/')
def show_dashboard():
    global topic_data
    plots = []
    for topic, data in topic_data.items():
        plots.append(make_ajax_plot(topic, data))
    dash = render_template('dashboard.html', plots=plots)
    return dash


def make_ajax_plot(topic, data):
    #topic_escaped = topic.replace('/', '&sol;')
    source = AjaxDataSource(data_url=request.url_root + 'data/{}'.format(topic),
                            polling_interval=2000, mode='replace')

    source.data = dict(x=data[0], y=data[1])
    plot = figure(plot_height=300, sizing_mode='scale_width', title=topic)
    plot.line('x', 'y', source=source, line_width=4)
    plot.title.text = topic
    plot.title.text_font_size = '20px'

    script, div = components(plot)
    return script, div


def make_plot(topic, data):
    plot = figure(plot_height=300, sizing_mode='scale_width')
    x = data[0]
    y = data[1]
    plot.line(x, y, line_width=4)
    plot.title.text = topic
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
