from flask import Flask
from flask import render_template
from bokeh.plotting import figure
from bokeh.embed import components
from mqtt_relay import MQTTRelay


app = Flask(__name__)
mqtt_relay = MQTTRelay()

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

app.run(port=80)