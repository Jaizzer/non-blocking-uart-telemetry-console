#include "stm32f411xe.h"
#include <stdint.h>

// Set buffer size to 64 bytes
#define RB_SIZE 64

typedef struct {
    // Each element in buffer is 1 byte since its uint8_t
    uint8_t buffer[RB_SIZE];

    // Use 32-bit for the pointers
    volatile uint32_t head;
    volatile uint32_t tail;
} RingBuffer;

RingBuffer rx_fifo = {{0}, 0, 0};

void rb_write(uint8_t byte) {
    // Calculate the next position
    uint32_t next = (rx_fifo.head + 1) % RB_SIZE;

    // Check if the next
    if (next != rx_fifo.tail) {
        // Write the byte to the current slot
        rx_fifo.buffer[rx_fifo.head] = byte;

        // Move the head to the next byte
        rx_fifo.head = next;
    }
};

uint8_t rb_read(void) {
    // Get the current byte
    uint8_t byte = rx_fifo.buffer[rx_fifo.tail];

    // Move to the next byte
    rx_fifo.tail = (rx_fifo.tail + 1) % RB_SIZE;

    // Return the currently stored byte
    return byte;
}

int rb_is_empty(void) { return rx_fifo.tail == rx_fifo.head; }

int main(void) {
    // Enable GPIO A
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN_Msk;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // Make PA2 alternate function so that it can be controlled by the USART
    GPIOA->MODER &= ~GPIO_MODER_MODE2_Msk;
    GPIOA->MODER |= GPIO_MODER_MODE2_1;

    // Make PA3 alternate function so that it can be controlled by the USART
    GPIOA->MODER &= ~GPIO_MODER_MODE3_Msk;
    GPIOA->MODER |= GPIO_MODER_MODE3_1;

    // Wire PA2 to AF7 (0111) which maps to USART
    GPIOA->AFR[0] &= ~(15 << 8);
    GPIOA->AFR[0] |= (7 << 8);

    // Wire PA3 to AF7 (0111) which maps to USART
    GPIOA->AFR[0] &= ~(15 << 12);
    GPIOA->AFR[0] |= (7 << 12);

    // Enable USART2
    RCC->APB1ENR &= ~RCC_APB1ENR_USART2EN_Msk;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // Set the mantissa part of 104.1875
    USART2->BRR &= ~USART_BRR_DIV_Mantissa_Msk;
    USART2->BRR |= (104U << 4);

    // Set the fraction part of 104.1875, 0.1875 * 16 = 3
    USART2->BRR &= ~USART_BRR_DIV_Fraction_Msk;
    USART2->BRR |= (3 << 0);

    // Set the USART2 CR1 register values to 8N1 (no parity) Configuration
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_OVER8;

    // Enable interrupts for USART2 when it receives data
    USART2->CR1 |= USART_CR1_RXNEIE;

    // Wait for the hardware to be ready in sending data
    // This condition will only work if USART2's bit 7 is 1
    while (!(USART2->SR & USART_SR_TXE)) {
    };

    // Set the value to be sent by the UART
    USART2->DR = 38;

    // Set interrupt for USART2
    NVIC_SetPriority(USART2_IRQn, 1);
    NVIC_EnableIRQ(USART2_IRQn);

    while (1) {
    }
}

void USART2_IRQHandler(void) {
    // Check if the interrupt was caused by receiving a data
    if (USART2->SR & USART_SR_RXNE) {
        uint8_t data = (uint8_t)USART2->DR;
        rb_write(data);
    }
}

/* Stub for -nostdlib compilation to satisfy the Reset_Handler in startup_stm32f411xe.s */
void __libc_init_array(void) { /* Empty stub */ }
