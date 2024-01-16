#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define ENABLE_USART2 USART2->CR1 |= USART_CR1_UE

void USART_configure(void);
void DMA_configure(void);
void NVIC_configure(void);
void I2C_configure(void);
void TIM_configure(void);
void RCC_configure(void);

#endif /* CONFIGURATION_H */
