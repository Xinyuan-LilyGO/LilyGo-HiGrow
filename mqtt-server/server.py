from flask import Flask
from flask.json import JSONEncoder
from flask import render_template, jsonify, request
import bokeh.palettes
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


DEFAULT_MQTT_BROKER = "ttgo-server.local"
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


class SensorInstanceData:
    def __init__(self, sensor_name: str) -> None:
        self.x_data = []
        self.y_data = []
        self.sensor_name = sensor_name

    def __repr__(self) -> str:
        return self.sensor_name


class SensorType:
    """
    A class that stores data series for a type of measurement (lux, temperature etc.)
    """

    def __init__(self, sensor_type: str) -> None:
        self.series_data = {}
        self.sensor_type = sensor_type

    def num_series(self) -> int:
        return len(self.series_data)

    def add_series(self, sensor_name: str) -> SensorInstanceData:
        sensor_instance_data = SensorInstanceData(sensor_name)
        self.series_data[sensor_name] = sensor_instance_data
        return sensor_instance_data

    def __repr__(self) -> str:
        return self.sensor_type


def sensor_type_and_name_from_topic(topic: str):
    # sensor type is the last part in the topic
    topic_parts = topic.split("/")
    sensor_name_str = topic_parts[-2]
    sensor_type_str = topic_parts[-1]
    return sensor_type_str, sensor_name_str


def new_topic_callback(topic):
    # if a new topic comes in
    sensor_type_str, sensor_name_str = sensor_type_and_name_from_topic(topic)
    if sensor_type_str not in topic_data:
        sensor_type = SensorType(sensor_type_str)
        sensor_type.add_series(sensor_name_str)
        topic_data[sensor_type_str] = sensor_type
    else:
        sensor_type: SensorType = topic_data[sensor_type_str]
        if sensor_name_str not in sensor_type.series_data:
            sensor_type.add_series(sensor_name_str)
        else:
            # this topic does exist, this was called erroneously
            pass


def new_data_callback(topic, data):
    # for now, all data is sent as formatted strings
    data_str = data.decode('utf-8')
    data_float = float(data_str)
    database.write_message(topic, data_float)
    print("New data: {} on topic {}".format(data_float, topic))

    sensor_type_str, sensor_name_str = sensor_type_and_name_from_topic(topic)
    if sensor_type_str in topic_data:
        sensor_type: SensorType = topic_data[sensor_type_str]
        if sensor_name_str in sensor_type.series_data:
            sensor_instance_data: SensorInstanceData = sensor_type.series_data[sensor_name_str]
            sensor_instance_data.x_data.append(datetime.now())
            sensor_instance_data.x_data.append(float(data_float))
        else:
            print("No space to put the new data!")
            pass


app = Flask(__name__)
app.json_encoder = CustomJSONEncoder


@app.route('/topics/')
def get_topics():
    return jsonify(database.get_topics())


@app.route('/data/<sensor_type>/<sensor_name>/', methods=['POST'])
def get_data(sensor_name, sensor_type):
    global topic_data
    if sensor_type in topic_data:
        sensor_type_data: SensorType = topic_data[sensor_type]
        if sensor_name in sensor_type_data.series_data:
            sensor_instance_data: SensorInstanceData = sensor_type_data.series_data[sensor_name]
            return jsonify(x=sensor_instance_data.x_data, y=sensor_instance_data.y_data)

    return jsonify(x=[], y=[])


@app.route('/dashboard/')
def show_dashboard():
    global topic_data
    plots = []
    for sensor_data in topic_data.values():
        plots.append(make_ajax_plot(sensor_data))
    dash = render_template('dashboard.html', plots=plots)
    return dash


def make_ajax_plot(sensor_data: SensorType):

    colours = bokeh.palettes.Category10[10]
    plot = figure(plot_height=300, sizing_mode='scale_width', title=topic)
    i = 0
    for sensor_instance_name, sensor_instance_data in sensor_data.series_data.items():
        update_path = 'data/{}/{}'.format(sensor_data.sensor_type,
                                          sensor_instance_name)
        source = AjaxDataSource(data_url=request.url_root + update_path,
                                polling_interval=30000, mode='replace')
        source.data = dict(x=sensor_instance_data.x_data,
                           y=sensor_instance_data.y_data)
        plot.line('x', 'y',
                  source=source,
                  line_width=4,
                  legend_label=sensor_instance_name,
                  color=colours[i % 10])
        i += 1

    plot.title.text = sensor_data.sensor_type
    plot.title.text_font_size = '20px'
    plot.legend.location = "top_left"
    plot.legend.click_policy = "hide"
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
    argparser.add_argument("--port", dest="flask_port",
                           help="The port that the web interface is served on", type=int, default=DEFAULT_FLASK_PORT)
    argparser.add_argument(
        "--db", dest="db_path", help="Path to the database file to be used", type=str, default=DEFAULT_DB_PATH)
    argparser.add_argument("--broker", dest="mqtt_broker",
                           help="The MQTT broker URI", type=str, default=DEFAULT_MQTT_BROKER)
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

    # first off, get all existing data from the database
    topics = database.get_topics()
    for topic in topics:
        data = database.get_data(topic)

        # sensor type is the last part in the topic
        sensor_type_str, sensor_name_str = sensor_type_and_name_from_topic(
            topic)

        if sensor_type_str in topic_data:
            sensor_type: SensorType = topic_data[sensor_type_str]
            the_data = sensor_type.add_series(sensor_name_str)
        else:
            sensor_type = SensorType(sensor_type_str)
            the_data = sensor_type.add_series(sensor_name_str)
            topic_data[sensor_type_str] = sensor_type

        x_series_data = the_data.x_data
        y_series_data = the_data.y_data
        for d in data:
            dt = datetime.strptime(d[0], '%Y-%m-%d %H:%M:%S')
            x_series_data.append(dt)
            y_series_data.append(d[1])
            if len(x_series_data) > MAX_DATA_LENGTH:
                x_series_data = x_series_data[1:]
                y_series_data = y_series_data[1:]

    # start the MQTT relay
    relay = MQTTRelay(
        topic_filter="sensor0/+",
        mqtt_host=args.mqtt_broker)
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
