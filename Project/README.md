# Task A3 – Controlling mouse cursor using accelerometer

## Objectives
 - Implement mouse cursor controlling on your computer screen
by board deflections
 - Deflection of the board is determined using accelerometer
 - Reading from accelerometer is initiated by counter
 - Information about change of the cursor position is sent to
computer using serial interface (UART)
 - On the computer side there is a simple script that changes the
location of the mouse cursor based on data read from serial
port
 - communication over I2C using interrupts
 - counter triggering an interrupt
 - UART handled by DMA and its interrupt