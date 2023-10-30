# TASK 2 - Implement non-blocking function writing diagnostic messages over the serial interface

## Objectives
 - Use DMA and its interrupt to handle UART
 - Implement handling buttons LEFT, RIGHT, UP, DOWN,
FIRE, USER, AT MODE using interrupts
 - Each pressing and releasing of a button (also caused by
contact oscillation) should cause the corresponding diagnostic
message (as in task 1) to be sent over the serial interface
 - Do not perform long-term or blocking operations in the
interrupt handler, e.g. do not wait for transfer completion of
the message
 - The current state of the button is to be recognized by reading
the register IDR
 - Avoid unnecessary copying