# --- 1. Project Names ---
# TARGET is just a variable. It defines the base name of your output files.
TARGET = application

# --- 2. Toolchain ---
# CC: The C Compiler. 'none-eabi' means no OS (Bare Metal).
CC      = arm-none-eabi-gcc
# OBJCOPY: Converts the "heavy" ELF file (with debug info) into a "slim" BIN file.
OBJCOPY = arm-none-eabi-objcopy
# SIZE: A utility that reports how many bytes of Flash and RAM your code uses.
SIZE    = arm-none-eabi-size

# --- 3. Hardware Settings ---
# -mcpu: Tells the compiler to use instructions specific to the Cortex-M4.
# -mthumb: Tells the CPU to use the 16-bit "Thumb" instruction set (required for Cortex-M).
MCU = -mcpu=cortex-m4 -mthumb

# --- 4. Search Paths (The "GPS" for Headers) ---
# We define where the 'Rosetta Stone' .h files live so we can use -I later.
CMSIS_CORE = ./CMSIS_5/CMSIS/Core/Include
CMSIS_DEV  = ./cmsis_device_f4/Include
SYSTEM_DIR = ./cmsis_device_f4/Source/Templates
STARTUP_DIR = ./cmsis_device_f4/Source/Templates/gcc

# --- 5. Compiler Flags (The "How-to-Build" Rules) ---
# -g: Includes debug symbols (essential for troubleshooting).
# -std=gnu11: Uses the 2011 C standard with GNU extensions.
# -O0: Optimization Level 0. Prevents the compiler from "simplifying" your code.
# -ffreestanding: Assumes there is no standard library (no printf/malloc).
CFLAGS  = $(MCU) -g -std=gnu11 -O0 -ffreestanding
# -D: Defines a macro. This tells the headers "I am a F411xE chip."
CFLAGS += -DSTM32F411xE
# -I: Adds the directories we defined earlier to the search path for #include.
CFLAGS += -I$(CMSIS_CORE) -I$(CMSIS_DEV)

# --- 6. Linker Flags (The "Blueprint" Rules) ---
# -T: Points to your linker.ld file to know where Flash and RAM start.
# -nostdlib: Tells the linker not to put 'libc' since this will require you to have the function the OS usually provides
# -nostartfiles: Tells the linker "Don't use the default C startup; I'm providing my own .s file."
LDFLAGS = -T linker.ld -nostdlib -nostartfiles

# --- 7. The Source Files (The Trinity) ---
# Here we list all three components: logic, hardware setup, and the assembly bootstrapper.
SRCS  = main.c
SRCS += $(SYSTEM_DIR)/system_stm32f4xx.c
SRCS += $(STARTUP_DIR)/startup_stm32f411xe.s

# --- 8. Build Rules (The "Action" Phase) ---

# 'all' is the default goal. It triggers the .bin creation and shows the size.
all: $(TARGET).bin size

# This rule compiles and links all SRCS into one .elf file.
# $@ is a shortcut for the target name (application.elf).
$(TARGET).elf: $(SRCS)
	@echo "Linking the Trinity into $@"
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $@

# This rule takes the .elf and strips it down to a raw .bin for the chip.
# $< is a shortcut for the first prerequisite (application.elf).
$(TARGET).bin: $(TARGET).elf
	@echo "Stripping metadata to create raw binary..."
	$(OBJCOPY) -O binary $< $@

# Custom command to flash the board.
flash: $(TARGET).bin
	st-flash --connect-under-reset write $(TARGET).bin 0x08000000

# Shows you the memory usage in the terminal.
size: $(TARGET).elf
	@$(SIZE) $(TARGET).elf

# Cleans up the folder so you can do a fresh build.
clean:
	rm -f *.elf *.bin *.o

# Tells Make that these names are commands, not actual files on your disk.
.PHONY: all clean flash size