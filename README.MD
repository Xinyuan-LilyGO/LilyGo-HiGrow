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

### The numbers

## Soil

This is a measure of the moisture content of the soil.

A `TLC555DR` timer generates a square wave which is output through a `10k` resistor, then through the large copper "pill" shaped pour (labelled "bytton" on the schematic), through a diode, to the `Humi` net which is connected to pin `IO32` on the `ESP32`, in parrallel with a `1uF` capacitor to ground (itself in parrallel with a resistor of `1M`).

Using equations `(8)` from the datasheet and seeing that `Ra = R34 = 300`, `Rb = R35 = 1600` and `Ct = C60 = 470pF` (and using some estimated values for the other parameters) we find the frequency is around `760kHz`.

Treating the "bytton" pad as a capacitor to ground whose capacitance changes with the moisture content of the surroundings, the voltage at pin `IO32` decreases from `~2.7V` (at `0.1pF`) to `0V` (at `~55uF`):

(See the LTSpice model in `\electronics\`)

![](electronics/Humivoltage.png)

The dielectric constant of water is `80.4` [2] and dry soil is around `4` [1], so presumably, as the moisture content of the soil increases, the dielectric constant, and thus capacitance of the "bytton" increases, and so the voltage drops.

- [1] [Dean et. al, 1987, SOIL MOISTURE MEASUREMENT BY AN IMPROVED CAPACITANCE TECHNIQUE, PART I. SENSOR DESIGN AND PERFORMANCE](https://www.sciencedirect.com/science/article/abs/pii/0022169487901946)
- [2] Sears, F. W., Zemansky, M. W., Young, H. D., University Physics, 6th Ed., Addison-Wesley, 1982.

### 3D file

![](https://github.com/Xinyuan-LilyGO/TTGO-Multi-function-sensor-board/blob/master/image/image2.png)
