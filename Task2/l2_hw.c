#include "header.h"

typedef struct
{
    GPIO_TypeDef *gpio;
    uint32_t reg;
    char *message_press;
    char *message_release;
    uint32_t neg;
} button_t;

static struct
{
    char *buffer[MAXSIZE];
    int32_t read_pos;
    int32_t insert_pos;
    int32_t used;
} messages;

static button_t controller_buttons[CONTROLLER_BUTTONS_NUMBER] = {
    {GPIOB, 3, MSG(STR_LEFT, RELEASED), "LEFT RELEASED\r\n", 0},
    {GPIOB, 4, MSG(STR_RIGHT, RELEASED), "RIGHT RELEASED\r\n", 0},
    {GPIOB, 5, "UP PRESSED\r\n", "UP RELEASED\r\n", 0},
    {GPIOB, 6, "DOWN PRESSED\r\n", "DOWN RELEASED\r\n", 0},
    {GPIOB, 10, "FIRE PRESSED\r\n", "FIRE RELEASED\r\n", 0},
    {GPIOC, 13, "USER PRESSED\r\n", "USER RELEASED\r\n", 0},
    {GPIOA, 0, "MODE PRESET\r\n", "MODE RELEASED\r\n", 1}};

// --------------------- Queue ---------------------

// Clear the Message Queue:
// sets all the fields of the messages struct to 0
static void clear_queue(void)
{
    messages.read_pos = 0;
    messages.insert_pos = 0;
    messages.used = 0;
}

// Check if the Message Queue is empty:
// returns 1 if the queue is empty, 0 otherwise
static int32_t is_queue_empty(void)
{
    return messages.used == 0;
}

// Check if the Message Queue is full:
// returns 1 if the queue is full, 0 otherwise
static int32_t is_queue_full(void)
{
    return messages.used == MAXSIZE;
}

static char *queue_poll(void)
{
    char *ptr = messages.buffer[messages.read_pos];
    messages.read_pos = (messages.read_pos + 1) % MAXSIZE;
    messages.used--;
    return ptr;
}

// Push a message to the Message Queue:
static void queue_push(char *ptr)
{
    messages.buffer[messages.insert_pos] = ptr;
    messages.insert_pos = (messages.insert_pos + 1) % MAXSIZE;
    messages.used++;
}

// --------------------- Configures ---------------------

static void configure_button(button_t *button)
{
    GPIOinConfigure(button->gpio,
                    button->reg,
                    GPIO_PuPd_UP,
                    EXTI_Mode_Interrupt,
                    EXTI_Trigger_Rising_Falling);
}

// Configure RCC:
// Code from Slide 9 (w8)
static void RCC_configure(void)
{
    // Enable GPIOA, GPIOB, GPIOC, DMA1 clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN |
                    RCC_AHB1ENR_DMA1EN;

    // Enable USART2 clock
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // Enable SYSCFG clock
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
}

// Configure UART2:
// Code from Slides 10 to 11 (w8)
static void UART_configure(void)
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
static void DMA_configure(void)
{
    /* USART2 TX:
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

    /* USART2 RX:
        uses stream 5 and channel 4, direct transfer mode, 8-bits transfers,
        high priority, increasing the memory
        address after every transfer, interrupt after transfer
        completion
    */
    DMA1_Stream5->CR = 4U << 25 |
                       DMA_SxCR_PL_1 |
                       DMA_SxCR_MINC |
                       DMA_SxCR_TCIE;

    // Set the peripheral address
    DMA1_Stream5->PAR = (uint32_t)&USART2->DR;

    DMA1->HIFCR = DMA_HIFCR_CTCIF6 |
                  DMA_HIFCR_CTCIF5;
}

// Configure NVIC:
static void NVIC_configure(void)
{
    // Code from Slides 14 (w8)
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    NVIC_EnableIRQ(DMA1_Stream5_IRQn);

    // Code from Slides 28 (w5)
    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI3_IRQn);
    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

// --------------------- Handlers ---------------------

static uint32_t is_pressed(button_t *button)
{
    return ((button->gpio->IDR >> button->reg) & 1) ^ button->neg;
}

// Starting sending
// Code from Slide 15 (w8)
static void send_to_DMA1(char *message)
{
    DMA1_Stream6->M0AR = (uint32_t)message;
    DMA1_Stream6->NDTR = strlen(message);
    DMA1_Stream6->CR |= DMA_SxCR_EN;
}

static void receive_from_DMA1(char *message)
{
    DMA1_Stream5->M0AR = (uint32_t)message;
    DMA1_Stream5->NDTR = strlen(message);
    DMA1_Stream5->CR |= DMA_SxCR_EN;
}

// Interrupt handler:
// Solution to problem on Slide 18 (w8)
static void interrupt_handler(uint32_t EXTI_PR_STATE,
                              uint32_t LINE_INTERRUPT_STATE,
                              button_t *button)
{
    if (EXTI_PR_STATE & LINE_INTERRUPT_STATE)
    {
        // Write message according to button pressed/released state
        char *message = is_pressed(button)
                            ? button->message_release
                            : button->message_press;

        // If the bits EN and TCIFx are cleared, the transfer can be initiated
        if ((DMA1_Stream6->CR & DMA_SxCR_EN) == 0 &&
            (DMA1->HISR & DMA_HISR_TCIF6) == 0)
        {

            send_to_DMA1(message);
        }
        // If queue not full, push message to queue
        // "If this condition is not met, the transfer must be queued"
        // the condition being: DMA1_Stream6->CR & DMA_SxCR_EN == 0 && DMA1->HISR & DMA_HISR_TCIF6 == 0
        else if (!is_queue_full())
        {
            queue_push(message);
        }

        // There is an event triggering an interrupt
        EXTI->PR = LINE_INTERRUPT_STATE;
    }
}

// Template of interrupt handler after send completion
void DMA1_Stream6_IRQHandler(void)
{
    // Read signalled DMA1 interrupts
    uint32_t isr = DMA1->HISR;

    if (isr & DMA_HISR_TCIF6)
    {
        // Handle transfer completion on stream 5
        DMA1->HIFCR = DMA_HIFCR_CTCIF6;

        // If there is something to send, start next transfer
        if (!is_queue_empty())
        {
            send_to_DMA1(queue_poll());
        }
    }
}

// Template of interrupt handler after receive completion
void DMA1_Stream5_IRQHandler()
{
    // Read signalled DMA1 interrupts
    uint32_t isr = DMA1->HISR;

    if (isr & DMA_HISR_TCIF5)
    {
        // Handle transfer completion on stream 5
        DMA1->HIFCR = DMA_HIFCR_CTCIF5;
       
        // Enable receiving again. 
        receive_from_DMA1(queue_poll());
    }
}

// External interrupt:
// A set bit in the EXTI->PR register means that there is an event which can trigger an interrupt

// Button 6 (USER) Register 0
void EXTI0_IRQHandler(void)
{
    uint32_t interrupt_state = EXTI->PR;
    interrupt_handler(interrupt_state, EXTI_PR_PR0, &controller_buttons[6]);
}

// Button 0 (LEFT) Register 3
void EXTI3_IRQHandler(void)
{
    uint32_t interrupt_state = EXTI->PR;
    interrupt_handler(interrupt_state, EXTI_PR_PR3, &controller_buttons[0]);
}

// Button 1 (RIGHT) Register 4
void EXTI4_IRQHandler(void)
{
    uint32_t interrupt_state = EXTI->PR;
    interrupt_handler(interrupt_state, EXTI_PR_PR4, &controller_buttons[1]);
}

// Buttons 2, 3 (UP, DOWN) Register 5, 6
void EXTI9_5_IRQHandler(void)
{
    uint32_t interrupt_state = EXTI->PR;
    interrupt_handler(interrupt_state, EXTI_PR_PR5, &controller_buttons[2]);
    interrupt_handler(interrupt_state, EXTI_PR_PR6, &controller_buttons[3]);
}

// Buttons 4, 5 (FIRE, MODE) Register 10, 13
void EXTI15_10_IRQHandler(void)
{
    uint32_t interrupt_state = EXTI->PR;
    interrupt_handler(interrupt_state, EXTI_PR_PR10, &controller_buttons[4]);
    interrupt_handler(interrupt_state, EXTI_PR_PR13, &controller_buttons[5]);
}

// --------------------- Main ---------------------

int main(void)
{
    clear_queue();

    RCC_configure();

    __NOP();

    UART_configure();
    DMA_configure();
    NVIC_configure();

    for (int i = 0; i < CONTROLLER_BUTTONS_NUMBER; ++i)
    {
        configure_button(controller_buttons + i);
    }

    ENABLE_PERIPHERAL;

    for (;;)
    {
    }

    return 0;
}