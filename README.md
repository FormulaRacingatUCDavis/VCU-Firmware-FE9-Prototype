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
#### Hardware
1. PICDuino microcontroller
2. Breadboard circuit with the following components shown below
3. A hex wrench to turn the potentiometers
4. USB-A to USB-B cable to connect PICDuino to your computer

![vcu-circuit](https://user-images.githubusercontent.com/72328335/151675667-8e6ae435-11d8-482e-8f56-c0d1faa9b7b1.png)
This shows the proper mapping of the breadboard circuit to the PICDuino. You must connect the jumper cables to the right pins in order for the code to work.

*Note: if the controls are not reading values properly, try changing the pins they are connected to. Afterwards, you have to modify the pin configuration in MPLAB X IDE and change the code to use the right pins.*

#### Software
To print to the serial monitor over UART:
1. Open MCC in MPLAB X IDE
2. Go to Resource Management -> UART1
3. Under the software settings, check the box that redirects STDIO to UART
4. Apply changes

### How to use
*These directions are with respect to the image.*
- The potentiometers are turned from right (min value=0) to left (max value=4095) to increase the value.
- The switches are configured with left state being off (0) and right state being on (1).
