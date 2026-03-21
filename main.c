#include "stm32f411xe.h"
#include <stdint.h>

int main(void) {
    // Enable GPIO A
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN_Msk;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

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

    // Wait for the hardware to be ready in sending data
    // This condition will only work if USART2's bit 7 is 1
    while (!(USART2->SR & USART_SR_TXE)) {
    };

    // Set the value to be sent by the UART
    USART2->DR = 38;

    while (1) {
    }
}

/* Stub for -nostdlib compilation to satisfy the Reset_Handler in startup_stm32f411xe.s */
void __libc_init_array(void) { /* Empty stub */ }
