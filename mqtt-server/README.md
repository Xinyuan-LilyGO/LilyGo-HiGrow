Please run `activate_venv.bat` before doing anything.

## Useful links
- `https://davidhamann.de/2018/02/11/integrate-bokeh-plots-in-flask-ajax/` how to update Bokeh graphs in real time

# Broker setup

A raspberry pi is set up as the server.  It is
 - running the MQTT broker (mosquitto)
 - running `server.py` as a service
 - running an mDNS service


## Setting up the mDNS service

```
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install avahi-daemon
```
Open `/etc/hosts` and change the line `127.0.1.1` to the local domain name you want to use.  In this case we use `ttgo-server`.

Open `/etc/hostname` and change the line to `ttgo-server`

Then reboot
```
sudo reboot
```

The server will now be addressable via `ttgo-server.local`

- https://www.howtogeek.com/167190/how-and-why-to-assign-the-.local-domain-to-your-raspberry-pi/
- https://www.howtogeek.com/167195/how-to-change-your-raspberry-pi-or-other-linux-devices-hostname/