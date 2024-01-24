#include <gpio.h>
#include <stm32.h>
#include <delay.h>
#include "consts.h"
#include "configuration.h"

// USART Constants
#define BAUD_RATE 9600U
#define HSI_HZ 16000000U
#define PCLK1_HZ HSI_HZ

// I2C Constants
#define I2C_SPEED_HZ 100000
#define PCLK1_MHZ 16
#define CTRL_REG1_VALUE 0b01000111
#define CTRL_REG3_VALUE 0b00000100

// Wait Max Time
#define WAIT_MAX 1000000

// TIM Constants
#define PSC_VALUE 400
#define ARR_VALUE 1000

// Configure USART2:
// Code from Slides 10 to 11 (w8)
void USART_configure(void)
{
    GPIOafConfigure(GPIOA,
                    2,
                    GPIO_OType_PP,
                    GPIO_Fast_Speed,
                    GPIO_PuPd_NOPULL,
                    GPIO_AF_USART2);

    GPIOafConfigure(GPIOA,
                    3,
                    GPIO_OType_PP,
                    GPIO_Fast_Speed,
                    GPIO_PuPd_UP,
                    GPIO_AF_USART2);

    USART2->CR1 = USART_CR1_RE | USART_CR1_TE;
    USART2->CR2 = 0;
    USART2->BRR = (PCLK1_HZ + (BAUD_RATE / 2U)) / BAUD_RATE;

    // Sending and Receceiving using DMA
    USART2->CR3 = USART_CR3_DMAT | USART_CR3_DMAR;
}

// Configure DMA1:
// Code from Slides 12 to 14 (w8)
void DMA_configure(void)
{
    /* USART2 TX (sending stream):
        uses stream 6 and channel 4, direct transfer mode, 8-bits transfers,
        high priority, increasing the memory
        address after every transfer, interrupt after transfer
        completion
    */
    DMA1_Stream6->CR = 4U << 25 |
                       DMA_SxCR_PL_1 |
                       DMA_SxCR_MINC |
                       DMA_SxCR_DIR_0 |
                       DMA_SxCR_TCIE;

    // Set the peripheral address
    DMA1_Stream6->PAR = (uint32_t)&USART2->DR;

    DMA1->HIFCR = DMA_HIFCR_CTCIF6;
}

void NVIC_configure()
{
    // Code from Slides 14 (w8)
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);

    NVIC_EnableIRQ(I2C1_EV_IRQn);

    // 16-bit General-purpose timer
    NVIC_EnableIRQ(TIM3_IRQn);
}

// Wait for a condition to be true:
// if the condition is not true after
// WAIT_MAX iterations, STOP
static void wait_condition(uint16_t condition)
{
    volatile uint32_t counter = 0;

    while (!(I2C1->SR1 & condition))
    {
        ++counter;

        if (counter > WAIT_MAX)
        {
            I2C1->CR1 |= I2C_CR1_STOP;
            return;
        }
    }
}

// Writting to the accelerometer register:
// Code from Slides 19 and 20 (w7)
static void I2C_configure_partial(uint8_t slave_register_number, uint8_t value)
{
    // Initiate the transmission of the START signal
    I2C1->CR1 |= I2C_CR1_START;

    // Wait until completed
    // Signaled by the SB bit in the SR1 register
    wait_condition(I2C_SR1_SB);

    // Send slave address
    I2C1->DR = LIS35DE_ADDR << 1;

    // Wait until completed
    wait_condition(I2C_SR1_ADDR);

    // Read SR2 to clear ADDR
    I2C1->SR2;

    // Send the register number
    I2C1->DR = slave_register_number;

    // Wait until completed
    wait_condition(I2C_SR1_TXE);

    // Insert value into transmit queue
    I2C1->DR = value;

    // Wait until completed
    wait_condition(I2C_SR1_BTF);

    // Transmit STOP signal
    I2C1->CR1 |= I2C_CR1_STOP;
}

// Configure I2C:
// Code from Slides 16 and 17 (w7)
void I2C_configure()
{
    GPIOafConfigure(GPIOB,
                    8,
                    GPIO_OType_OD,
                    GPIO_Low_Speed,
                    GPIO_PuPd_NOPULL,
                    GPIO_AF_I2C1);

    GPIOafConfigure(GPIOB,
                    9,
                    GPIO_OType_OD,
                    GPIO_Low_Speed,
                    GPIO_PuPd_NOPULL,
                    GPIO_AF_I2C1);

    // Configure bus in the basic version
    I2C1->CR1 = 0;

    // Configure bus clock frequency
    I2C1->CCR = (PCLK1_MHZ * 1000000) /
                (I2C_SPEED_HZ << 1);
    I2C1->CR2 = PCLK1_MHZ;
    I2C1->TRISE = PCLK1_MHZ + 1;

    // Enable the interface
    I2C1->CR1 |= I2C_CR1_PE;

    __NOP();

    // Main configuration register
    I2C_configure_partial(I2C_CTRL_REG1, CTRL_REG1_VALUE);

    //Delay(100000);

    // Interrupt configuration register
    // I2C_configure_partial(I2C_CTRL_REG3, CTRL_REG3_VALUE);
}

void TIM_configure()
{
    // Enable counting
    TIM3->CR1 = 0;

    // Set Prescaler
    TIM3->PSC = PSC_VALUE;

    // Set Auto-reload register
    // Counts from 0 to 1000
    TIM3->ARR = ARR_VALUE;

    // Update generation
    TIM3->EGR = TIM_EGR_UG;

    // Clear the status register
    TIM3->SR = ~(TIM_SR_UIF | TIM_SR_CC1IF);

    // Enable interrupts
    TIM3->DIER = TIM_DIER_UIE | TIM_DIER_CC1IE;

    // capture/compare register
    TIM3->CCR1 = 500;

    // Start the timer
    TIM3->CR1 |= TIM_CR1_CEN;
}

// Configure RCC:
// Code from Slide 9 (w8)
void RCC_configure()
{
    // Enable GPIOA, GPIOB, GPIOC, DMA1 clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN |
                    RCC_AHB1ENR_DMA1EN;

    // Enable USART2, I2C, TIM3 clock
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN |
                    RCC_APB1ENR_I2C1EN |
                    RCC_APB1ENR_TIM3EN;

    // Enable SYSCFG clock
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
}

void USART_enable(){
    USART2->CR1 |= USART_CR1_UE;
}