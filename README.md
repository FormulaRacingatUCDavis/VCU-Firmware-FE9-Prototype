# Vehicle Control Unit

This is the *prototype* code for the vehicle control unit, a board that manages the soft state of the car. It hosts a finite state machine (FSM) to control this state. This code can be run on a PICDuino microcontroller to test the code.

## Completed
- Breadboard circuit for PICDuino

## In-progress
- Finite state machine
  - Get precharging state from motor controller
  - Send torque requests to motor controller
  - and more...

## How to setup breadboard for PICDuino
![IMG_3884 1](https://user-images.githubusercontent.com/72328335/150894564-20cec0b3-8b81-4c29-87c8-1ceceed3ae96.JPG)
This shows the proper mapping of the breadboard circuit to the PICDuino. You must connect the jumper cables to the right pins in order for the code to work.
