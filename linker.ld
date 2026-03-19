/* --- 1. Memory Definitions --- */
/* Defines the physical boundaries of the STM3F411 chip */
MEMORY 
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Define the end of RAM to use as the Initial Stack pointer since the stack grows downward */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* --- 2. Sections Mapping --- */
SECTIONS
{   
    /* -----------------------------
     * This is the most important block in your firmware. On power-up, the 
     * STM32 hardware is hard-wired to look at address 0x08000000. 
     * It expects to find the "Vector Table" here.
     *
     * IMPORTANT: The actual data (words) for this section are defined in 
     * your ASSEMBLY file (usually startup_stm32f411xe.s). This linker 
     * block just grabs that assembly data and places it at the very front.
     */
    .isr_vector :
    {
        /* * 1. . = ALIGN(4);
         * The "Snap to Grid" rule. ARM CPUs are 32-bit and grab data in 4-byte 
         * chunks. If the current address is 0x08000002, this command adds 
         * 2 bytes of padding to push it to 0x08000004. If it's at 0x08000003, 
         * it adds 1 byte. This ensures the table starts on a clean 'lane'.
         */
        . = ALIGN(4);

        /* * 2. KEEP(*(.isr_vector))
         * THE PROTECTOR: Tells the Linker NOT to delete this section. 
         * Since the hardware uses this table (not the software), the 
         * linker's garbage collector would otherwise throw it away.
         */
        KEEP(*(.isr_vector))

        /* * 3. . = ALIGN(4);
         * The "Exit Alignment". Ensures that whatever follows this table 
         * (your code) also starts on a multiple of 4. This prevents 
         * 'misaligned access' which can slow down the CPU or cause crashes 
         * when jumping to your functions.
         */
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        /* * 1. . = ALIGN(4);
         * The "Snap to Grid" rule.
         */
        . = ALIGN(4);

        /* 2. *(.text): The "Standard Bucket"
         * Grabs the default code blocks from every object file.
         */
        *(.text)

        /* 3. *(.text*): The "Wildcard Bucket"
         * Grabs sub-sections like .text.main or .text.Reset_Handler.
         */
        *(.text*)

        /* 4. glue_7 and glue_7t: THE BITE-SIZE GEARBOX 
         * This is where the "Plus 1" is used to toggle the CPU's internal 
         * Mode Switch, determining if it swallows 16 or 32 bits.
         */

        /* * 4a. .glue_7: ARM to Thumb (The 16-bit Swallow) */
        *(.glue_7)

        /* * 4b. .glue_7t: Thumb to ARM (The 32-bit Swallow) */
        *(.glue_7t)

        /* 5. .rodata: The "Immutable" Bucket 
         * This is for constants, like: const int MAX_SPEED = 500;
         */
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)

        . = ALIGN(4);
        /* * THE BOOKMARK: This marks the exact byte where the "Code" ends and the "Initial Data" begins.
         */
        _etext = .;
    } > FLASH

    /* * The .data section contains global and static variables that have 
     * an initial value (e.g., int score = 50;). 
     *
     * VMA (Virtual Memory Address): > RAM  (Where the code looks for it)
     * LMA (Load Memory Address):    AT > FLASH (Where it is stored permanently)
     */
    .data : 
    {
        . = ALIGN(4);      /* 1. Move the '.' to the next 4-byte boundary for speed */
        
        _sdata = .;        /* 2. Capture the current RAM address into '_sdata'. 
                              NOTE: This name must exactly match the destination 
                              label used in your assembly startup code. */

        *(.data)           /* 3. Pull in all .data sections from every .o file */
        *(.data*)          /* 4. Pull in extra data (like those from -ffunction-sections) */

        . = ALIGN(4);      /* 5. Pad the end so we always copy full 4-byte "words" */
        
        _edata = .;        /* 6. Capture the final RAM address. 
                              NOTE: This name must exactly match the end-of-section 
                              label used in your assembly loop condition. */
    } > RAM AT > FLASH

    /* * _sidata is the "Warehouse" address. 
     * LOADADDR is a special function that returns the physical FLASH address.
     * NOTE: This name must exactly match the source label used in your 
     * assembly 'ldr' instruction to find the data in Flash.
     */
    _sidata = LOADADDR(.data);
    
     /* -----------------------------
     * .bss Section: The "Empty Room"
     * This contains global/static variables that start at zero (e.g., int x = 0;).
     * * THE EFFICIENCY RULE: Since we know these must be zero, we don't store 
     * them in FLASH. This section takes up ZERO space in your binary file.
     * The Startup Assembly loop simply "wipes" this RAM area at boot.
     */
    .bss :
    {
        . = ALIGN(4);      /* 1. Snap to 4-byte grid for fast 32-bit clearing */
        
        /* 2. THE HANDSHAKE: _sbss is your "Private" name. 
         * It must exactly match the label in your startup_stm32.s loop.
         */
        _sbss = .;         

        /* 3. THE STANDARD: __bss_start__ is the "Public" name.
         * Many C libraries (like libc) expect this name to exist. 
         * It points to the exact same RAM address as _sbss.
         */
        __bss_start__ = _sbss; 

        *(.bss)            /* 4. Pull in all standard .bss sections */
        *(.bss*)           /* 5. Pull in wildcard .bss sections (e.g., .bss.main) */
        *(COMMON)          /* 6. Pull in 'common' symbols from older C code */

        . = ALIGN(4);      /* 7. Final alignment padding */
        
        /* 8. THE STOP SIGNAL: _ebss tells your assembly loop when to quit.
         * __bss_end__ provides the same "Public" alias for libraries.
         */
        _ebss = .;         
        __bss_end__ = _ebss;

    } > RAM                /* NO "AT > FLASH" - We generate zeros in RAM on the fly! */

    /* -----------------------------
     * THE TRASH CAN: Discard metadata the ARM chip doesn't need.
     * .note.GNU-stack: A Linux security flag (useless on bare metal).
     * .gnu_debuglink: A link to external debug files (useless in the .bin).
     */
    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.gnu_debuglink)
        *(.gnu.lto)
    }


    /* -----------------------------
     * ._user_heap_stack: The "Building Inspector"
     * This is a "Safety Net" that prevents your code from even being written 
     * to the board if there isn't enough RAM for it to actually run.
     * * THE LOGIC: It doesn't store data; it "pretends" to use extra RAM to 
     * ensure your Stack (local variables) has room to "breathe" without 
     * smashing into your Global variables.
     */
    ._user_heap_stack :
    {
        . = ALIGN(8);      /* 1. ARM Standard: Stack must be on an 8-byte grid */
        
        /* 2. THE DYNAMIC START: 'end' and '_end' tell malloc() exactly 
         * where the "Free RAM" begins (right after your globals).
         */
        PROVIDE ( end = . );
        PROVIDE ( _end = . );

        /* 3. THE RESERVATION: Pushing the "dot" forward to claim space.
         * 0x200 (512 bytes) reserved for the Heap (dynamic memory).
         * 0x400 (1024 bytes) reserved for the Stack (function calls).
         */
        . = . + 0x200;     
        . = . + 0x400;     

        . = ALIGN(8);      /* 4. Final alignment for the "Ghost" boundary */

    } > RAM                /* If (Globals + Heap + Stack) > 128KB, the build FAILS here. */
}