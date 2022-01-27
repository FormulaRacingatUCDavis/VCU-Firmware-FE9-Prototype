# Vehicle Control Unit

This is the *prototype* code for the vehicle control unit, a board that manages the overall state of the car. It uses a finite state machine (FSM) to control this state. Although this code is intended to be run on its original hardware, it can be run on a PICDuino microcontroller to physically test it using breadboard controls.

## Progress
### Completed
- Breadboard circuit for PICDuino

### In-progress
- Finite state machine
  - Get precharging state from motor controller
  - Send torque requests to motor controller
  - and more...

## Breadboard circuit for PICDuino
This circuit simulates some of the real controls in the car. These controls are:
- Brake pedal
- Gas pedal
- High voltage switch
- Drive switch

### How to setup
1. PICDuino microcontroller
2. Breadboard circuit with the following components shown below
3. A hex wrench to turn the potentiometers
4. USB-A to USB-B cable to connect PICDuino to your computer

![IMG_3884 1](https://user-images.githubusercontent.com/72328335/150894564-20cec0b3-8b81-4c29-87c8-1ceceed3ae96.JPG)
This shows the proper mapping of the breadboard circuit to the PICDuino. You must connect the jumper cables to the right pins in order for the code to work.

### How to use
*These directions are with respect to the image.*
- The potentiometers are turned from right (min value=0) to left (max value=4095) to increase the value.
- The switches are configured with left state being off (0) and right state being on (1).
