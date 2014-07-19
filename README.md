letterbox
=========

Sensing deliveries in your physical letter box

## Stucture

- `/ble` - Arduino [BLE](http://en.wikipedia.org/wiki/Bluetooth_low_energy) using [nRF8001](http://www.nordicsemi.com/eng/Products/Bluetooth-R-low-energy/nRF8001)
- `/sensors` - Sensing light and proximity using [Arduino Uno](http://arduino.cc/en/Main/arduinoBoardUno), [Arduino IDE 1.5.7](http://arduino.cc/en/main/software), LDRs and [VCNL 4000](http://www.adafruit.com/products/466)
- `/android` - Android App

## References

1. Sensors
	- [VCNL 4000 with Arduino Uno](https://github.com/adafruit/VCNL4000/blob/master/vcnl4000.pde)
	- [Smoothing](http://playground.arduino.cc/main/smooth) raw jittery sensor data
	- [Low Pass Filter](http://en.wikipedia.org/wiki/Low-pass_filter) algorithm
	- [Smoothing sensor data with low pass filter](http://blog.thomnichols.org/2011/08/smoothing-sensor-data-with-a-low-pass-filter)
	- Test functionality of a [photo resistor with Arduino](http://playground.arduino.cc/Learning/PhotoResistor)
	
## Install

- Sensors: copy the folder `/sensors/letterbox` to `~/Documents/Arduino/letterbox`

## Hack all the things!

