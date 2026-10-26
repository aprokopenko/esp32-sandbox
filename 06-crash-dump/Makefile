.PHONY: default coredump.read backtrace.explain

PORT ?= /dev/ttyACM1
PIOENV ?= ttgo-lora32-v21-dev
COREDUMP_FILE := .pio/coredump.bin
COREDUMP_INFO := .pio/coredump.txt
GDB := $(HOME)/.platformio/packages/tool-xtensa-esp-elf-gdb/bin/xtensa-esp32-elf-gdb

# coredump partition from the partition table (default.csv)
COREDUMP_OFFSET ?= 0x3F0000
COREDUMP_SIZE ?= 0x10000

default:
	@echo 'Crash dump helper'

# Remember to turn off serial monitor!!
coredump.read:
	@echo "Dumping coredump partition..."
	@echo "-------------------------------"
	esptool --port $(PORT) read-flash $(COREDUMP_OFFSET) $(COREDUMP_SIZE) ${COREDUMP_FILE}
	@echo "=============================="
	@echo ""
	@echo "Decoding coredump partition..."
	@echo "-------------------------------"
	esp-coredump info_corefile -c ${COREDUMP_FILE} -t raw \
	   --gdb ${GDB} \
	   .pio/build/${PIOENV}/firmware.elf \
	   > ${COREDUMP_INFO}

backtrace.explain:
	@test -n "${ADDR}" || { echo "usage: make backtrace.explain ADDR=<failure addr>"; exit 1; }
	xtensa-esp32-elf-addr2line -pfiaC -e .pio/build/${PIOENV}/firmware.elf ${ADDR}