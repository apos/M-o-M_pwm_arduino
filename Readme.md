# FUN with your Mirror-o-Matic (12/24V)
 
Program for a custom made "Mirror-o-Matic" machine controlling two motors 
(turntable and ExtenderWheel) with and arduino (nano) and pwm.
We are using I2C for input devices like the 4x4 keypad and the 16x2 lcd screen.
We are using HM-10 for bluetooth communication
We are using TCST2103 transmissive optical sensors for controlling the speed of the motors.
We are using BTS7960 43A H-bridges for controlling an 12V and an 24V motor (3A)with PWM.

Axel Pospischil
Homepage http://wiki.blue-it.org
Project: https://github.com/apos/M-o-M_pwm_arduino
Copyright (GNU public licence v. 3.0): 

Read the Changelog.md and Readme.md files contained in the source folder.

# Usage:

The software starts by default in Timer-Mode.
By default the turntable is selected.

## Use case

1. software starts in turntable mode (if not altered otherwise by pressing "*")
2. select a speed in turntable mode ( second line ">TBL" ) by selecting [1-9]
3. change to extender wheel mode by pressing "*" ( first line changes to  ">EXT" )
4. select a speed for the extender wheel with [1-9]
5. Enable the timer (default) by pressing "D" (a "T" is visible at the end of the second line)
5. set a time for the timer with [A]
   short press: + 0.5 minutes
   long press:  + 5 minutes
6. start the process with [C] (continue)
7. you can press [0] during the operation and resume with [D]. The timer keeps counting, either you are in timer mode ("T" or not "nT").

## 4x4 Keypad 

*Button A:* short: add 0.5 minutes to timer
            long:  add 5 minutes to timer

*Button B:* short : sub -0.5 minutes from timer
            long:  add 5 minutes to timer

*Button C:* resume the actual run (time must be added with button A)

*Button D:* toggle Timer on / off 

**:*        toggle extender wheel / turntable mode

*\#:*       use with caution! 
            long  press:  ALL processes run to STOP ... AND
                           ALL values of the session will be reset to 0 and are lost.

*[1-9]:*    enter speed for given mode.
            In timer mode ("T") this only sets the values. The machine will only start running after pressing "C". In no-timer mode ("nT") the machine will start at once. Changes in speed when the machine is running are applied at once.

*[0]:*      all processes run to 0. You can resume with [D]


## Code

### Simulation mode for testing
This makes it possible to test the software without connection to any PWM- or INPUT pins.

Set global var: int SIMULATION = 1;
Or press:       [\*] (this will be removed in version 1.0 !)

### Other

All other variables changes that can be made by the user are documented in the source code.

