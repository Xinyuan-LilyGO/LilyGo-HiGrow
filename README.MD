TTGO HiGrow
===========================

![](image/T-Higrow.jpg)


## Operation

Use `platformio` to compile and upload firmware to the device.  The firmware periodically takes measurements using sensors of the device (see `The measurements` below), and every few measurements connects to the local Wifi access point to broadcast them to an MQTT broker at address `ttgo-server.local`.

See the README in `mqtt-server` for how to set up and run the server.


## Button operations
- *Long press* the *BOOT* button to enter smartconfig mode
- *Long press* the *User* button to enter deepsleep mode
- *Double press* the User button to enter smartconfig mode
- *Press* the *Reset* button to reset the device

## The measurements

### Soil

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