
#include <gpio.h>
#include <irq.h>
#include <stm32.h>
#include <string.h>

#define BAUD_RATE 9600U
#define HSI_HZ 16000000U
#define PCLK1_HZ HSI_HZ

#define CONTROLLER_BUTTONS_NUMBER 7

#define STR_LEFT "LEFT"
#define STR_RIGHT "RIGHT"
#define STR_UP "UP"
#define STR_DOWN "DOWN"
#define STR_FIRE "FIRE"
#define STR_USER "USER"
#define STR_MODE "MODE"

#define PRESSED " PRESSED\r\n"
#define RELEASED " RELEASED\r\n"

#define MSG(BUTTON, TYPE) BUTTON ## #TYPE

#define MAXSIZE 512

#define ENABLE_PERIPHERAL USART2->CR1 |= USART_CR1_UE;