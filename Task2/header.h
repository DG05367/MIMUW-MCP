#ifndef HEADER_H
#define HEADER_H

#include <gpio.h>
#include <irq.h>
#include <stm32.h>
#include <string.h>

#define BAUD_RATE 9600U
#define HSI_HZ 16000000U
#define PCLK1_HZ HSI_HZ

#define CONTROLLER_BUTTONS_NUMBER 7

#define MAXSIZE 512

#define ENABLE_PERIPHERAL USART2->CR1 |= USART_CR1_UE

#endif