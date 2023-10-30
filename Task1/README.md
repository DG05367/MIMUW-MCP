# TASK 1 - Implement communication with the board over the serial interface

## Objectives

 - It should be possible to control the state of the four LEDs
(switch on, switch off, toggle) using the following commands:
LR1, LR0, LRT, LG1, LG0, LGT, LB1, LB0, LBT, Lg1, Lg0,
LgT

 - Invalid commands should be ignored

 -  Each button pressing or releasing should cause that the board sends the appropriate message, each message on a separateline: 
 LEFT PRESSED, LEFT RELEASED, RIGHT PRESSED,
 RIGH RELEASED, UP PRESSED, UP RELEASED, DOWN
 PRESSED, DOWN RELEASED, FIRE PRESSED, FIRE
 RELEASED, USER PRESSED, USER RELEASED, MODE
 PRESET, MODE RELEASED

 - There are 7 buttons (together with joystick)

## Additional requirements

 - Do not suppress multiple press or release detection caused by
contact vibration
 - Receiving commands and sending messages by the board
should not block each other

## Algorithm Sketch

 - Implement a cyclic send buffer

 - Implement a receive buffer, not necessarily cyclical

 - In an infinite loop, execute

    - if there is a character to read in the UART receive buffer, read
it and save it to the receive buffer

    - if there is an LED command in the receive buffer, execute it

    - if any button changed state, insert appropriate message into
the transmit buffer

    - if the transmit buffer is non-empty and the UART transmit
buffer is empty, insert character to send