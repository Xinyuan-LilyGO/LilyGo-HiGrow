from flask import Flask
from flask.json import JSONEncoder
from flask import render_template, jsonify, request
from bokeh.models.sources import AjaxDataSource
from bokeh.models.formatters import DatetimeTickFormatter
from bokeh.plotting import figure
from bokeh.embed import components
from mqtt_relay import MQTTRelay
import threading
import argparse
import os
from datetime import datetime
import logging
import database


DEFAULT_FLASK_PORT = 1234
DEFAULT_DB_PATH = os.path.join("databases", "database.db")
MAX_DATA_LENGTH = 5000
topic_data = {}
database = database.Database()


class CustomJSONEncoder(JSONEncoder):
    def default(self, obj):
        try:
            if isinstance(obj, datetime):
                # bokeh graphs need milliseconds since epoch as the datetime
                return (obj - datetime.utcfromtimestamp(0)).total_seconds() * 1000
            iterable = iter(obj)
        except TypeError:
            pass
        else:
            return list(iterable)
        return JSONEncoder.default(self, obj)


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
        the_data = topic_data[topic]
        the_data[0].append(datetime.now())
        the_data[1].append(float(data_float))


app = Flask(__name__)
app.json_encoder = CustomJSONEncoder


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
        #plots.append(make_plot(topic, data))
    dash = render_template('dashboard.html', plots=plots)
    return dash


def make_ajax_plot(topic, data):
    source = AjaxDataSource(data_url=request.url_root + 'data/{}'.format(topic),
                            polling_interval=10000, mode='replace')

    source.data = dict(x=data[0], y=data[1])
    plot = figure(plot_height=300, sizing_mode='scale_width', title=topic)
    plot.line('x', 'y', source=source, line_width=4)
    plot.title.text = topic
    plot.title.text_font_size = '20px'
    fs_days = "%Y-%m-%d"
    fs_hours = "%Y-%m-%d %H"
    fs_mins = "%Y-%m-%d %H:%M"
    fs_secs = "%Y-%m-%d %H:%M:%S"
    plot.xaxis.formatter = DatetimeTickFormatter(
        seconds=[fs_secs],
        minutes=[fs_mins],
        hours=[fs_hours],
        days=[fs_days],
        months=[fs_days],
        years=[fs_days],
    )

    script, div = components(plot)
    return script, div


def make_plot(topic, data):
    plot = figure(plot_height=300, sizing_mode='scale_width')
    x = data[0]
    y = data[1]
    plot.line(x, y, line_width=4)
    plot.title.text = topic
    plot.title.text_font_size = '20px'
    fs_days = "%Y-%m-%d"
    fs_hours = "%Y-%m-%d %H"
    fs_mins = "%Y-%m-%d %H:%M"
    fs_secs = "%Y-%m-%d %H:%M:%S"
    plot.xaxis.formatter = DatetimeTickFormatter(
        seconds=[fs_secs],
        minutes=[fs_mins],
        hours=[fs_hours],
        days=[fs_days],
        months=[fs_days],
        years=[fs_days],
    )
    script, div = components(plot)
    return script, div


if __name__ == "__main__":

    argparser = argparse.ArgumentParser()
    argparser.add_argument("--port", dest="flask_port", help="The port that the web interface is served on", type=int, default=DEFAULT_FLASK_PORT)
    argparser.add_argument("--db", dest="db_path", help="The port that the web interface is served on", type=str, default=DEFAULT_DB_PATH)
    args = argparser.parse_args()

    # make database instance
    db_path = args.db_path
    database_path = os.path.dirname(db_path)
    database_name = os.path.basename(db_path)
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
        for d in data:
            dt = datetime.strptime(d[0], '%Y-%m-%d %H:%M:%S')
            the_data[0].append(dt)
            the_data[1].append(d[1])
            if len(the_data[0]) > MAX_DATA_LENGTH:
                the_data[0] = the_data[0][1:]
                the_data[1] = the_data[1][1:]
        topic_data[topic] = the_data

    # start the MQTT relay
    relay = MQTTRelay(topic_filter="otsensor/+")
    relay.register_new_topic_callback(new_topic_callback)
    relay.register_new_data_callback(new_data_callback)
    relay.initialise()

    start_flask_app_blocking = True
    if start_flask_app_blocking:
        app.run(port=args.flask_port,
        debug=False,
        use_reloader=False,
        host='0.0.0.0')
    else:
        threading.Thread(target=app.run, kwargs={
                     'port': args.flask_port,
                     'debug': False,
                     'use_reloader': False,
                     'host': '0.0.0.0'}).start()
    print("Flask server started...")

    # close things
    relay.uninitialise()
    database.close()
