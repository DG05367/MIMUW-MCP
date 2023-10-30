#include <delay.h>
#include <gpio.h>
#include <stm32.h>
#include <stdlib.h>
#include <string.h>

#define RED_LED_GPIO GPIOA
#define GREEN_LED_GPIO GPIOA
#define BLUE_LED_GPIO GPIOB
#define GREEN2_LED_GPIO GPIOA
#define RED_LED_PIN 6
#define GREEN_LED_PIN 7
#define BLUE_LED_PIN 0
#define GREEN2_LED_PIN 5

#define RedLEDon() \
    RED_LED_GPIO->BSRR = 1 << (RED_LED_PIN + 16)
#define RedLEDoff() \
    RED_LED_GPIO->BSRR = 1 << RED_LED_PIN

#define GreenLEDon() \
    GREEN_LED_GPIO->BSRR = 1 << (GREEN_LED_PIN + 16)
#define GreenLEDoff() \
    GREEN_LED_GPIO->BSRR = 1 << GREEN_LED_PIN

#define BlueLEDon() \
    BLUE_LED_GPIO->BSRR = 1 << (BLUE_LED_PIN + 16)
#define BlueLEDoff() \
    BLUE_LED_GPIO->BSRR = 1 << BLUE_LED_PIN

#define Green2LEDon() \
    GREEN2_LED_GPIO->BSRR = 1 << GREEN2_LED_PIN
#define Green2LEDoff() \
    GREEN2_LED_GPIO->BSRR = 1 << (GREEN2_LED_PIN + 16)

#define HSI_HZ 16000000U
#define PCLK1_HZ HSI_HZ
#define BAUD_RATE 9600U

#define USART_WordLength_8b 0x0000
#define USART_Parity_No 0x0000
#define USART_Enable USART_CR1_UE
#define USART_FlowControl_None 0x0000
#define USART_StopBits_1 0x0000

#define USART_Mode_Rx_Tx (USART_CR1_RE | \
                          USART_CR1_TE)

#define SEND_BUFFER_SIZE  512
#define RECV_BUFFER_SIZE  128

#define BUTTON_NUMS 7