#include "stm32f411xe.h"
#include <stdint.h>

// Make the Ring Buffer 64-bytes long
#define RB_SIZE 64

typedef struct {
    // Use uint8_t since each buffer item are 8-bits (1 byte) long
    uint8_t buffer[RB_SIZE];

    // This pointer is the one used for writing
    volatile uint32_t head;

    // This pointer is the one used for reading
    volatile uint32_t tail;
} RingBuffer;

// {0} fills the bugger array with zero
// While the two other zeros sets the head and tail pointer to zero
RingBuffer rx_fifo = {{0}, 0, 0};

void rb_write(uint8_t byte) {
    // Get the next slot after head pointer and use modulo to make the direction cyclical
    uint32_t next = (rx_fifo.head + 1) % RB_SIZE;

    // If the next pointer and the tail would not overlap, then there is still a slot
    // We leave the last spot as open to distinguish between an empty and a full ring buffer
    if (next != rx_fifo.tail) {
        // Store the byte to the buffer
        rx_fifo.buffer[rx_fifo.head] = byte;

        // Advance the head pointer
        rx_fifo.head = next;
    }
}

uint8_t rb_read(void) {
    // Read the byte that the tail currently points to
    uint8_t byte = rx_fifo.buffer[rx_fifo.tail];

    // Advance the tail pointer
    rx_fifo.tail = (rx_fifo.tail + 1) % RB_SIZE;

    // Return the byte
    return byte;
}

// This function checks whether the ring buffer is empty
int rb_is_empty(void) { return rx_fifo.tail == rx_fifo.head; }

int main(void) {
    // 1. Enable GPIOA, and GPIOC and USART6 Clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART6EN;

    // 2. Configure PA5 (LED)
    GPIOA->MODER &= ~GPIO_MODER_MODE5_Msk;
    GPIOA->MODER |= GPIO_MODER_MODE5_0;

    // First, "erase" the current Job Description for PC6 and PC7.
    // GPIO_MODER_MODE6_Msk is 0b11. Using &= ~ forces both bits to 00 (Input mode).
    // This ensures we have a clean slate before assigning a new mode.
    GPIOC->MODER &= ~(GPIO_MODER_MODE6_Msk | GPIO_MODER_MODE7_Msk);

    // Now, assign the 'Alternate Function' job description (Binary 10).
    // We use the _1 suffix because it targets the "left-hand" bit of the 2-bit pair.
    // This tells the pins: "You are no longer general GPIO; you are now specialists."
    GPIOC->MODER |= (GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1);

    // Map to AF8 (USART6), clear the bits though.
    GPIOC->AFR[0] &= ~(0xF << 24 | 0xF << 28);

    // PC6 takes the second "4-bits" starting at bit 4, PC7 takes the third "4-bits" starting
    // at bit 8
    GPIOC->AFR[0] |= (8 << 24) | (8 << 28);

    // 4. Configure USART6 Baud Rate (9600 @ 16MHz)
    // The BRR (Baud Rate Register) is a frequency divider.
    // Calculation for 9600 baud with a 16MHz clock:
    // USARTDIV = 16,000,000 / (16 * 9600) = 104.1667
    //
    // Mantissa (Whole number) = 104 -> This goes into bits [15:4]
    // Fraction = 0.1667 * 16 (since the fraction slot is 4-bits wide) = 2.667
    // We round 2.667 up to 3 -> This goes into bits [3:0]
    USART6->BRR = (104U << 4) | (3U << 0);

    // 5. CR1 Config: Master Enable, turn on Talk (TX) and Listen (RX),
    // and enable the "New Data" Interrupt so we don't have to wait manually.
    USART6->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;

    // 6. NVIC Setup: The "Secretary" that manages CPU interruptions
    // Set priority to 1 (High priority). 0 is the highest, 15 is the lowest.
    // This ensures UART data is handled quickly even if the CPU is busy.
    NVIC_SetPriority(USART6_IRQn, 1);

    // Open the gate for USART6 interrupts. Without this, the CPU will
    // ignore the signals coming from the USART6 peripheral.
    NVIC_EnableIRQ(USART6_IRQn);

    // Startup Signal (Blink LED)
    GPIOA->BSRR = GPIO_BSRR_BS5;

    // Wait for some second to turn the LED off again
    for (int i = 0; i < 1000000; i++)
        __asm__("nop");
    GPIOA->BSRR = GPIO_BSRR_BR5;

    // Send a startup byte (ASCII '&') to signal successful initialization
    // Wait until the Transmit Data Register is Empty (TXE bit becomes 1)
    while (!(USART6->SR & USART_SR_TXE))
        ;

    // Load the character into the Data Register to begin transmission
    USART6->DR = '&';

    while (1) {
        // This while loop will keep running, waiting for ring buffer to be non-empty
        // 1. Check the "Inbox" (Ring Buffer)
        // If head != tail, the interrupt has dropped off new data from the Mac.
        if (!rb_is_empty()) {

            // 2. Retrieve the oldest byte from the software warehouse (RAM).
            uint8_t data = rb_read();

            /* --- ECHO BACK LOGIC --- */

            // 3. Wait for the "Loading Dock" (Transmit Data Register) to be clear.
            // The TXE bit is 1 when empty. We stay in this loop as long as TXE is 0 (Busy).
            while (!(USART6->SR & USART_SR_TXE))
                ;

            // 4. "Return to Sender": By writing the byte back into the DR,
            // the hardware automatically pulses it out the TX wire (PA9) to your Mac.
            USART6->DR = data;

            /* --- COMMAND LOGIC --- */

            // 5. Action Phase: Check if the character is a specific command.
            if (data == 'H') {
                // Bit Set: Turns PA5 (LED) ON
                // BR5 corresponds to Bit Set 5
                GPIOA->BSRR = GPIO_BSRR_BS5;
            } else if (data == 'I') {
                // Bit Reset: Turns PA5 (LED) OFF
                // BR5 corresponds to Bit Reset 5
                GPIOA->BSRR = GPIO_BSRR_BR5;
            }
        }
    }
}

/**
 * @brief  Interrupt Service Routine for USART6.
 * This function is called automatically by the hardware
 * whenever a UART event (like receiving a byte) occurs.
 */

// This gets triggered whenever a new data is received from the computer
void USART6_IRQHandler(void) {

    // 1. Check the Status Register (SR) for the 'Read Data Register Not Empty' flag (RXNE).
    //    This confirms that the interrupt was triggered because a byte was actually received.
    // This condition AND condition will only become 00000000 if  the bit corresponding to
    // USART_SR_RXNE in SR are not equal
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