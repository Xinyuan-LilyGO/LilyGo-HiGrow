TTGO HiGrow
===========================

![](image/T-Higrow.jpg)

### Wifi Setup
If the device is powered on for the first time, or after the firmware has been reflashed, it will start to broadcast its own wifi network.
You can connect to this wifi network and go to `192.168.4.1` where the device broadcasts its Dash dashboard.

To connect to an existing WiFi network, *Double press* the User button to put the device into smart-config mode, and then configure using [EspTouch] (https://www.espressif.com/en/products/software/esp-touch/resources).
When the configuration is successfull, the device will store the SSID and PSK of the network in non-volatile memory, and connect to the network.  Check your router etc. to find out what address the device has been given.

When the device is next restarted, it will find the SSID and PSK that it stored previously, and try to reconnect to that network.

To make the device start broadcasting its own WiFI network again, enter smart-config mode (by *Double pressing* the *User* button), and then press the *Reset* button.


### Peripheral description
- *Long press* the *BOOT* button to enter smartconfig mode
- *Long press* the *User* button to enter deepsleep mode
- *Double press* the User button to enter smartconfig mode
- *Press* the *Reset* button to reset the device

### The DASH board

![](image/1.png)


### 3D file

![](https://github.com/Xinyuan-LilyGO/TTGO-Multi-function-sensor-board/blob/master/image/image2.png)
