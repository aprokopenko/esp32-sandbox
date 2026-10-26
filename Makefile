.PHONY: default init pio esptools gdb deps path serial-group

PIO_HOME := $(HOME)/.platformio
PIO      := $(PIO_HOME)/penv/bin/pio

# Group that owns /dev/ttyUSB*: uucp on Arch, dialout on Debian/Ubuntu
SERIAL_GROUP ?= uucp

default:
	@echo "Sandbox helper"

init: pio esptools gdb

# PlatformIO Core (same location the VS Code extension uses)
pio:
	python3 -m venv $(PIO_HOME)/penv
	$(PIO_HOME)/penv/bin/pip install --upgrade platformio

# esp-coredump (+ esptool as its dependency)
esptools:
	python3 -m venv $(PIO_HOME)/espenv
	$(PIO_HOME)/espenv/bin/pip install --upgrade esptool esp-coredump

# Standalone Xtensa GDB for esp-coredump: the one in toolchain-xtensa-esp32
# needs libpython2.7 and fails to start on modern distros
gdb:
	$(PIO) pkg install -g -t espressif/tool-xtensa-esp-elf-gdb

# Platforms, toolchains and libs for one project: make deps DIR=06-crash-dump
deps:
	@test -n "$(DIR)" || { echo "usage: make deps DIR=<project dir>"; exit 1; }
	$(PIO) pkg install -d $(DIR)

# Serial port access without sudo; takes effect after re-login
serial-group:
	sudo usermod -aG $(SERIAL_GROUP) $(USER)

# Run ONCE: appends to shell configs, a second run duplicates the lines.
# Appended (not prepended) so venv python3/pip don't shadow the system ones.
path:
	echo 'export PATH="$$PATH:$$HOME/.platformio/penv/bin:$$HOME/.platformio/espenv/bin:$$HOME/.platformio/packages/toolchain-xtensa-esp32/bin"' >> ~/.bashrc
	echo 'export PATH="$$PATH:$$HOME/.platformio/penv/bin:$$HOME/.platformio/espenv/bin:$$HOME/.platformio/packages/toolchain-xtensa-esp32/bin"' >> ~/.zshrc
	fish -c 'fish_add_path -U --append ~/.platformio/penv/bin ~/.platformio/espenv/bin ~/.platformio/packages/toolchain-xtensa-esp32/bin'
