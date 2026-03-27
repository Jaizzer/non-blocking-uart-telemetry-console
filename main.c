#include "stm32f411xe.h"
#include <stdint.h>

#define RB_SIZE 64

typedef struct {
    uint8_t buffer[RB_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} RingBuffer;

RingBuffer rx_fifo = {{0}, 0, 0};

void rb_write(uint8_t byte) {
    uint32_t next = (rx_fifo.head + 1) % RB_SIZE;
    if (next != rx_fifo.tail) {
        rx_fifo.buffer[rx_fifo.head] = byte;
        rx_fifo.head = next;
    }
}

uint8_t rb_read(void) {
    uint8_t byte = rx_fifo.buffer[rx_fifo.tail];
    rx_fifo.tail = (rx_fifo.tail + 1) % RB_SIZE;
    return byte;
}

int rb_is_empty(void) { return rx_fifo.tail == rx_fifo.head; }

int main(void) {
    // 1. Enable GPIOA and USART6 Clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART6EN;

    // 2. Configure PA5 (LED)
    GPIOA->MODER &= ~GPIO_MODER_MODE5_Msk;
    GPIOA->MODER |= GPIO_MODER_MODE5_0;

    // 3. Configure PA9 (TX) and PA10 (RX) for USART6
    GPIOA->MODER &= ~(GPIO_MODER_MODE9_Msk | GPIO_MODER_MODE10_Msk);
    GPIOA->MODER |= (GPIO_MODER_MODE9_1 | GPIO_MODER_MODE10_1);

    // Map to AF8 (USART6)
    GPIOA->AFR[1] &= ~(0xF << 4 | 0xF << 8);
    GPIOA->AFR[1] |= (8 << 4) | (8 << 8);

    // 4. Configure USART6 Baud Rate (9600 @ 16MHz)
    USART6->BRR = (104U << 4) | (3U << 0);

    // 5. CR1 Config: Enable USART, TX, RX, and RX Interrupt (No OVER8)
    USART6->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;

    // 6. NVIC Setup
    NVIC_SetPriority(USART6_IRQn, 1);
    NVIC_EnableIRQ(USART6_IRQn);

    // Startup Signal (Blink LED)
    GPIOA->BSRR = GPIO_BSRR_BS5;
    for (int i = 0; i < 1000000; i++)
        __asm__("nop");
    GPIOA->BSRR = GPIO_BSRR_BR5;

    // Send a startup byte (ASCII '&')
    while (!(USART6->SR & USART_SR_TXE))
        ;
    USART6->DR = '&';

    while (1) {
        if (!rb_is_empty()) {
            uint8_t data = rb_read();

            // Echo back
            while (!(USART6->SR & USART_SR_TXE))
                ;
            USART6->DR = data;

            // Command Logic
            if (data == 'H') {
                GPIOA->BSRR = GPIO_BSRR_BS5; // LED ON
            } else if (data == 'I') {
                GPIOA->BSRR = GPIO_BSRR_BR5; // LED OFF
            }
        }
    }
}

/**
 * @brief  Interrupt Service Routine for USART6.
 * This function is called automatically by the hardware
 * whenever a UART event (like receiving a byte) occurs.
 */
void USART6_IRQHandler(void) {

    // 1. Check the Status Register (SR) for the 'Read Data Register Not Empty' flag (RXNE).
    //    This confirms that the interrupt was triggered because a byte was actually received.
    // This condition AND condition will only become 00000000 if  the bit corresponding to USART_SR_RXNE in SR are not equal
    if (USART6->SR & USART_SR_RXNE) {

        // 2. Read the 8-bit data from the Data Register (DR).
        //    IMPORTANT: The act of reading USART6->DR automatically clears the RXNE flag
        //    in the hardware, telling the STM32 we have handled this byte.
        uint8_t received_byte = (uint8_t)USART6->DR;

        // 3. Place the received byte into our "sushi belt" (Ring Buffer).
        //    This allows the main loop to process the data later without
        //    slowing down the high-speed UART reception.
        rb_write(received_byte);
    }
}

void __libc_init_array(void) {}