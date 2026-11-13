# ESP32 sandbox

Learning projects for embedded development: ESP32 (plus one nRF52840) with **PlatformIO** and the **Arduino** framework. Each folder is a separate PlatformIO project with its own README: what it does, the wiring, and how it works.

## Examples

| Example | Board | What it is about |
|---|---|---|
| [01-blink](01-blink) | ESP32 DevKit v1 | Blinks the onboard LED from a periodic `esp_timer` callback instead of `delay()`. |
| [02-button-debounce](02-button-debounce) | ESP32 DevKit v1 | Compares three ways to read a push button: raw `digitalRead()`, the OneButton library, and a hand-written debounce class. |
| [03-motion-sensor-buzzer](03-motion-sensor-buzzer) | ESP32 DevKit v1 | A PIR motion sensor triggers a random note on a passive buzzer. |
| [04-ttgo-lora32-custom-spi](04-ttgo-lora32-custom-spi) | TTGO LoRa32 | Talks to the onboard SX1276 LoRa radio with SPI implemented by hand (bit-banging), without an SPI library. |
| [05-nrf-lora-transmitter](05-nrf-lora-transmitter) | XIAO nRF52840 | Sends a LoRa packet every 30 s with an SX1262 radio and RadioLib. The transmitting half of a pair. |
| [05-ttgo-lora-receiver](05-ttgo-lora-receiver) | TTGO LoRa32 | Receives those packets with the onboard SX1276 and shows them on the OLED. The receiving half of the pair. |
| [06-crash-dump](06-crash-dump) | TTGO LoRa32 | Crashes the ESP32 on purpose and finds out why: the decoded backtrace in the monitor, and the core dump read from flash. |
| [07-freertos-basics](07-freertos-basics) | TTGO LoRa32 | Splits the work into FreeRTOS tasks connected by queues: a single or double click sends its own LoRa packet. |
| [08-wifi-captive-portal](08-wifi-captive-portal) | TTGO LoRa32 | Wi-Fi setup through a captive portal, SNTP time, and an HTTPS notification on a button press. Built in layers: app, services, drivers. |

## Shared files

| Path | What it is |
|---|---|
| [ARCH_GUIDE.md](ARCH_GUIDE.md) | Summary of the lecture on firmware architecture (layers, drivers as interfaces, events, tasks). The reference for reviews from example 08 on. |
| [clean/](clean) | Empty PlatformIO project for an ESP32 DevKit v1, the starting point for a new example. |
| [Makefile](Makefile) | Tool setup: PlatformIO, esptool, esp-coredump, the Xtensa GDB, and serial port access. |
| [.clang-format](.clang-format) | Code style for all examples (Arduino style, 2 spaces, no alignment after an open bracket). |

## Getting started

On a new machine, run once from the repo root:

```sh
make init    # PlatformIO, esptool + esp-coredump, standalone Xtensa GDB
make path    # adds the tools to PATH (run only once)
```

Then build and flash any example from its folder:

```sh
cd 07-freertos-basics
pio run -t upload
pio device monitor
```

Some examples need more steps, for example uploading a file system image. Their READMEs say so.
