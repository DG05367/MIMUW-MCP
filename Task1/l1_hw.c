#include "header.h"

// Messages have 2* button numbers size, 7 released 7 pressed
static const char *MESSAGES[2 * BUTTON_NUMS] = {
    "USER PRESSED\r\n",
    "USER RELEASED\r\n",
    "LEFT PRESSED\r\n",
    "LEFT RELEASED\r\n",
    "RIGHT PRESSED\r\n",
    "RIGHT RELEASED\r\n",
    "UP PRESSED\r\n",
    "UP RELEASED\r\n",
    "DOWN PRESSED\r\n",
    "DOWN RELEASED\r\n",
    "FIRE PRESSED\r\n",
    "FIRE RELEASED\r\n",
    "MODE PRESET\r\n",
    "MODE RELEASED\r\n"};

static const uint32_t MSG_LENGTHS[2 * BUTTON_NUMS] = {
    14, 15, 14, 15, 15, 16, 12, 13, 14, 15, 14, 15, 13, 15};

static __IO uint32_t
    red_led_state;
static __IO uint32_t
    green_led_state;
static __IO uint32_t
    blue_led_state;
static __IO uint32_t
    green2_led_state;

static uint32_t button_states[BUTTON_NUMS] = {0};
static uint32_t button_to_reg_map[BUTTON_NUMS] = {13, 3, 4, 5, 6, 10, 0};

static uint32_t send_buffer_pos = 0;
static uint32_t send_buffer_used = 0;
static uint32_t send_pos = 0;
static char send_buffer[SEND_BUFFER_SIZE];

static uint32_t get_reg_for_button(uint32_t button_num)
{
    if (button_num == 0)
    {
        return GPIOC->IDR;
    }
    else if (button_num < 6)
    {
        return GPIOB->IDR;
    }
    else
    {
        return GPIOA->IDR;
    }
}

static uint32_t get_message_index(uint32_t button_num)
{
    if (button_num < 6)
    {
        return 2 * button_num + button_states[button_num];
    }
    else
    {
        return 2 * button_num + 1 - button_states[button_num];
    }
}

// receives button number and returns his state
static uint32_t get_button_state_from_controller(uint32_t button_num)
{
    uint32_t reg_base = get_reg_for_button(button_num);
    uint32_t reg_offset = button_to_reg_map[button_num];
    return (reg_base >> reg_offset) & 1;
}

// parser for received buffer
static uint32_t parse_query(char *recv_buffer, uint32_t buffer_used)
{
    if (buffer_used < 2 && recv_buffer[0] != 'L')
    {
        return 1;
    }
    else if (buffer_used < 2)
    {
        return 0;
    }
    else if (buffer_used < 3 && recv_buffer[1] != 'R' && recv_buffer[1] != 'G' && recv_buffer[1] != 'B' && recv_buffer[1] != 'g')
    {
        return 1;
    }
    else if (buffer_used < 3)
    {
        return 0;
    }
    else if (buffer_used < 4 && recv_buffer[2] != '0' && recv_buffer[2] != '1' && recv_buffer[2] != 'T')
    {
        return 1;
    }
    else if (buffer_used < 4)
    {
        char LED_char = recv_buffer[1];
        char opt_char = recv_buffer[2];

        if (LED_char == 'R')
        {
            if (opt_char == '0')
            {
                RedLEDoff();
            }
            else if (opt_char == '1')
            {
                RedLEDon();
            }
            else
            {
                red_led_state = red_led_state ^ (1 << RED_LED_PIN | 1 << (RED_LED_PIN + 16));
                RED_LED_GPIO->BSRR = red_led_state;
            }
        }
        else if (LED_char == 'G')
        {
            if (opt_char == '0')
            {
                GreenLEDoff();
            }
            else if (opt_char == '1')
            {
                GreenLEDon();
            }
            else
            {
                green_led_state = green_led_state ^ (1 << GREEN_LED_PIN | 1 << (GREEN_LED_PIN + 16));
                GREEN_LED_GPIO->BSRR = green_led_state;
            }
        }
        else if (LED_char == 'B')
        {
            if (opt_char == '0')
            {
                BlueLEDoff();
            }
            else if (opt_char == '1')
            {
                BlueLEDon();
            }
            else
            {
                blue_led_state = blue_led_state ^ (1 << BLUE_LED_PIN | 1 << (BLUE_LED_PIN + 16));
                BLUE_LED_GPIO->BSRR = blue_led_state;
            }
        }
        else
        {
            if (opt_char == '0')
            {
                Green2LEDoff();
            }
            else if (opt_char == '1')
            {
                Green2LEDon();
            }
            else
            {
                green2_led_state = green2_led_state ^ ( 1 << (GREEN2_LED_PIN + 16) | 1 << GREEN2_LED_PIN);
                GREEN2_LED_GPIO->BSRR = green2_led_state;
            }
        }
    }

    return 2;
}

//
static void append_message(uint32_t button_num)
{
    uint32_t message_index = get_message_index(button_num);
    uint32_t message_len = MSG_LENGTHS[message_index];
    uint32_t i = 0;

    if (send_buffer_used + message_len > SEND_BUFFER_SIZE)
    {
        return;
    }

    while (MESSAGES[message_index][i] != '\0')
    {
        send_buffer[send_buffer_pos] = MESSAGES[message_index][i];
        __NOP();
        send_buffer_pos = (send_buffer_pos + 1) % SEND_BUFFER_SIZE;
        send_buffer_used++;
        i++;
    }

    /*for (uint32_t i = 0; i < message_len; ++i)
    {
        send_buffer[send_buffer_pos] = MESSAGES[message_index][i];
        __NOP();
        send_buffer_pos = (send_buffer_pos + 1) % SEND_BUFFER_SIZE;
        send_buffer_used++;
    }*/
}

// go through the 7 buttons and check their states according to controller
static void check_buttons_states()
{
    for (uint32_t i = 0; i < BUTTON_NUMS; ++i)
    {
        uint32_t state_on_controller = get_button_state_from_controller(i);

        if (state_on_controller != button_states[i])
        {
            button_states[i] = state_on_controller;
            append_message(i);
        }
    }
}

int main(void)
{
    char recv_buffer[RECV_BUFFER_SIZE];

    uint32_t recv_buffer_used = 0;

    uint32_t parser_return_value;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN;

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    __NOP();

    USART2->CR1 = USART_Mode_Rx_Tx |
                  USART_WordLength_8b |
                  USART_Parity_No;

    USART2->CR2 = USART_StopBits_1;
    USART2->CR3 = USART_FlowControl_None;

    USART2->BRR = (PCLK1_HZ + (BAUD_RATE / 2U)) /
                  BAUD_RATE;

    USART2->CR1 |= USART_Enable;

    __NOP();

    RedLEDoff();
    GreenLEDoff();
    BlueLEDoff();
    Green2LEDoff();

    red_led_state = 1 << RED_LED_PIN;
    green_led_state = 1 << GREEN_LED_PIN;
    blue_led_state = 1 << BLUE_LED_PIN;
    green2_led_state = 1 << (GREEN2_LED_PIN + 16);

    GPIOoutConfigure(RED_LED_GPIO,
                     RED_LED_PIN,
                     GPIO_OType_PP,
                     GPIO_Low_Speed,
                     GPIO_PuPd_NOPULL);

    GPIOoutConfigure(GREEN_LED_GPIO,
                     GREEN_LED_PIN,
                     GPIO_OType_PP,
                     GPIO_Low_Speed,
                     GPIO_PuPd_NOPULL);

    GPIOoutConfigure(BLUE_LED_GPIO,
                     BLUE_LED_PIN,
                     GPIO_OType_PP,
                     GPIO_Low_Speed,
                     GPIO_PuPd_NOPULL);

    GPIOoutConfigure(GREEN2_LED_GPIO,
                     GREEN2_LED_PIN,
                     GPIO_OType_PP,
                     GPIO_Low_Speed,
                     GPIO_PuPd_NOPULL);

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
                    GPIO_PuPd_NOPULL,
                    GPIO_AF_USART2);

    for (uint32_t i = 0; i < BUTTON_NUMS; ++i)
    {
        button_states[i] = get_button_state_from_controller(i);
    }

    __NOP();

    for (;;)
    {
        // character received
        if (USART2->SR & USART_SR_RXNE)
        {
            char c;
            c = USART2->DR;

            recv_buffer[recv_buffer_used] = c;
            recv_buffer_used++;
        }

        // parse received buffer
        parser_return_value = parse_query(recv_buffer, recv_buffer_used);

        if (parser_return_value != 0)
        {
            recv_buffer_used = 0;
        }

        check_buttons_states();

        // send character
        if (send_buffer_used > 0 && (USART2->SR & USART_SR_TXE))
        {
            USART2->DR = send_buffer[send_pos];
            __NOP();
            send_pos = (send_pos + 1) % SEND_BUFFER_SIZE;
            send_buffer_used--;
        }
    }
}