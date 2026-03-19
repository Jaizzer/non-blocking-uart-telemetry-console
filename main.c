#include "stm32f411xe.h"
#include <stdint.h>

int main(void) {
    // Enable GPIO A
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN_Msk;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // Enable USART2
    RCC->APB1ENR &= ~RCC_APB1ENR_USART2EN_Msk;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    while (1) {
    }
}

/* Stub for -nostdlib compilation to satisfy the Reset_Handler in startup_stm32f411xe.s */
void __libc_init_array(void) { /* Empty stub */ }
