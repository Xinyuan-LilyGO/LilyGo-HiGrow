from flask import Flask
from flask import render_template
from bokeh.plotting import figure
from bokeh.embed import components
from mqtt_relay import MQTTRelay
from PyQt5.QtWidgets import QApplication
import threading


def new_topic_callback(topic):
    print("New topic: {}\n".format(topic))


def new_data_callback(topic, data):
    print("New data: {} on topic {}\n".format(data, topic))


app = Flask(__name__)
qtapp = QApplication([])


@app.route('/dashboard/')
def show_dashboard():
    plots = []
    plots.append(make_plot())
    dash = render_template('dashboard.html', plots=plots)
    return dash


def make_plot():
    plot = figure(plot_height=300, sizing_mode='scale_width')
    x = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    y = [2**v for v in x]
    plot.line(x, y, line_width=4)
    script, div = components(plot)
    return script, div


if __name__ == "__main__":

    relay = MQTTRelay(topic_filter="otsensor/+")
    relay.new_data.connect(new_data_callback)
    relay.new_topic.connect(new_topic_callback)
    relay.initialise()

    threading.Thread(target=app.run, kwargs={
                     'port': 80, 'debug': False, 'use_reloader': False}).start()
    print("Server started...")

    # we need to run the Qt application in order to have Qt signals work properly
    print("Starting qt app...")
    qtapp.exec_()
