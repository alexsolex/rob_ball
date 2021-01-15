# rob_ball
My own made, reversed engineered, version of Mira, the robot made by Alonso Martinez

## Intro
Rob'Ball (aka Rob' or robby for intimate persons) is a portmanteau word, composed by the word Rob like in Robot, and the word ball like a ... OK, you know what I mean.

Robby is my very own attempt in reproduce the amazing Mira robot made by Alonso Martinez, who used to work for Pixar as animator.
Alonso made Mira a couple of years ago, and I just discovered by watching a Adam Savage’s Tested video last September ( https://www.youtube.com/watch?v=0vfuOW1tsX0 ).
Mira is so cute, and technically it was very interesting to see how it works and furthermore, how it can be done.

Unfortunately, I didn't found any documentation on how to make it, but the technical pieces of informations one can get in the Tested video.

So I decided to reverse engineered it as much as I can and I made everything on my own whitout author documentation. Not only my robot should try to look as cute as Mira, but I have to make it versatile to be programmed later to accomplish some other tasks.

This robot can be made by yourself if you have access to a 3D printer, and if you have some basics in electronic and soldering.

## 3D parts
I designed Rob' using autodesk fusion360. To design the 3 axis mecanics, I got inspired by Mira robot of course, and by 'Pia the robot' from 'pohukai' on Thingiverse ( https://www.thingiverse.com/thing:2929670 )
At this stage, my mounting is not easy as I would like because it is very hard to align holes for screw mounting. I may work on a v2 to make it easier to mount and maintain.

## Architecture
The robot should be able to move it's head according to 3 axis (turn left/right, go up and down, and tilt right and left), should display it's mood with moves, color lights and sounds. Last but not least, it should recognize faces and follow from look people.
Mira works on battery... what a challenge ! I decided that Rob' will be powered from external powersupply.

![This is schematic of parts implantation](docs/schematic_robball.png)

### Face Tracking
#### Raspberry Pi 3B+ + piCamera
Python script continuously do face tracking (single face detection). When a face is found, it computes distance and angle from the center of the camera view. Then it sends through serial communication the left/right and up/down angle to move to get the face closer to the center.
When face is lost for a couple of seconds, face tracking mode stops and send a stop face tracking signal to serial port.

### Actionners
#### Arduino 
An arduino nano is powered by 9 volts through Vin pin. (9 volt comes from a MT3608 power booster powered by 5V power supply.)
Arduino nano is responsible for moving the servos, driving addressable leds, and animating eyes.
#### Servo-motors
The 3 tiny LKY61 servos are drived by a PCA9685 pwm board. This board handle pwm signals and is drived by I2C communication from arduino.
#### Eyes
To make living eyes, I used 2 tiny 0.96inch OLED I2C screens. They both are drived on I2C from arduino.
#### Colors
I used two WS2812B addressable LEDs (I bought a cheap WS2812B strip led and cutted just two to have the possibility to mix the colors is needed). Those leds are drived directly by arduino.

### Communication
Raspberry Pi face tracking sends commands to arduino through serial communication (TX/RX pins, not USB as plugs takes too much space :) ). Unfortunately levels signals are not the same (3.3V for Raspberry and 5V for Arduino). So we need a level shifter which will adapt signal level for each side equipments.

### Power supply
The whole robot is powered with a raspberry pi power supply (5V 2.5A).
The 5V is dispatched to power raspberry pi through its microUSB plug, power the PCA9685.
Arduino through Vin needs at least 6V to work properly. So I powered it with a 9v comming from a MT3608 power booster.

## Software
### Raspberry
#### Installation
TODO
#### Configuration
TODO
#### Python script
TODO
### Arduino
#### Software
TODO
