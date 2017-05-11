# Changelog:

## 2017-05-10 - Version 0.5.1
* added a "simulation" mode ( global var: int SIMULATION = 1; ) was added.
  This makes it possible to test the Software without connection to any PWM- or INPUT pins.
  
  THIS WILL BE REMOVED IN VERSION 1.0:
  Simulation mode can be also triggered by long press "*".


## 2017-05-03 - Version 0.5
* Bluetooth soldured 
* Advanced usage of A and B in Timer mode (+/- 0.5 Minutes / 5 Minutes depending on short / long press of button)

* TODO: http://www.geothread.net/arduino-making-a-simple-bluetooth-data-logger/
* OR TODO: Simple data logger:  save the log every time, the motor stops OR every 5 seconds don't send it, just make it avaible trough display
* TODO: documentation online
* TODO: implemented and tested some Bluetooth code, but Bluetooth is disabled in loop()
* TODO: make the timer mode a little bit more precise using millis()
        however in praktikal usage this is not necassary.
        I tested periods with 45 minutes which gives me only 20 seconds difference.
        The task is not that time critical.

## 2017-05-02 - Version 0.4
* Storage of actual settings (struct) in eeprom so they will be kept upon disconnecting the device from power (using EEPROMAnything.h). BUT: storage of additional settings in other structs currently do not work. The struct for saving data in Memory "A" and "B" are already in the code.
* TIMER MODE added

* TODO: Bluetooth: setting data from external devices
* TODO: Bluetooth: logbook (stored as struct internally, read via serial and bluetooth within certain intervall) 


## 2017-04-22 - Version 0.3
* new: I2C keypad with PCF8574 and 
       see: https://www.bastelgarage.ch/index.php?route=blog/post/view&blog_post_id=13

* TODO: performance issue
* TODO: hardware soldering
* TODO: documentation

## 2017-02-25 - Version 0.2
* Exzenter & Turntable
* LCD with I2C, Keypad 4x4 (no I2C)
* BTS7960 43A H bridge (no mosfet any more)

* TODO: performance issue
* TODO: keypad i2c when board arrives
* TODO: hardware soldering / doku

2017-02-06 - Version 0.1
* Only Extender wheel Motor, LCD with I2C, Keypad 4x4 (no I2C), Mosfet
  Working with Mosfet was unreliable and highly unstable => get BTS7960 43A H bridge

* TODO: Register when motor stops (too much force from outside)
* TODO: Turntable