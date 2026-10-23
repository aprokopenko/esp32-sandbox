# XIAO nRF52840: LoRa transmitter

A Seeed XIAO nRF52840 example that **sends a LoRa packet every 30 seconds** with an **SX1262** radio module. The radio is driven with the [RadioLib](https://github.com/jgromes/RadioLib) library.

This is one half of a pair. The other half is [05-ttgo-lora-receiver](../05-ttgo-lora-receiver): a TTGO LoRa32 board with an **SX1276** radio that receives the packets and shows them on its OLED.

```mermaid
flowchart LR
    TX["XIAO nRF52840 + SX1262<br/>(transmitter)"] -->|"LoRa, 868.3 MHz"| RX["TTGO LoRa32 + SX1276<br/>(receiver)"]
    TX --> LOG["Serial log"]
    TX --> LED["RGB LED"]
```

The two boards use **different radio chips** (SX1262 and SX1276). They can still talk to each other, because both use the same LoRa modulation and the same settings.

## How it works

1. `setup()` sets the TCXO voltage, then starts the radio with the LoRa settings from [include/lora.h](include/lora.h). The green LED turns on when the radio is ready.
2. Every 30 seconds `loop()` builds a text message and starts sending it with `startTransmit()`. This call does not wait: the radio sends the packet in the background. The red LED turns on.
3. When the packet is sent, the radio raises the **DIO1** pin. RadioLib calls the `setFlag()` interrupt handler, which only sets the `transmittedFlag` flag.
4. `loop()` sees the flag, logs the result, calls `finishTransmit()` to turn the transmitter off, and turns the red LED off.

The first packet is sent right after start-up, and the next ones every 30 seconds after that.

### The message

```
Package #3 at 63s, T=27.5
```

| Part | Meaning |
|---|---|
| `#3` | Packet number, counted from 1 |
| `63s` | Transmitter uptime in seconds (`millis() / 1000`) |
| `T=27.5` | nRF52840 CPU temperature in °C, from `readCPUTemperature()` |

### LEDs

The XIAO RGB LED is **active `LOW`**: `LOW` turns a color on, `HIGH` turns it off.

| LED | Meaning |
|---|---|
| Green on | Radio init succeeded |
| Red on | A packet is being sent |

## Radio settings

Both boards must use the **same** LoRa settings, or the receiver will not see the packets. The transmitter keeps them in [include/lora.h](include/lora.h), and the receiver keeps them in its own `include/lora.h`.

| Setting | Value | Notes |
|---|---|---|
| Frequency | **868.3 MHz** | EU 868 band |
| Bandwidth | **250 kHz** | |
| Spreading factor | **7** | Lower = faster, shorter range |
| Coding rate | **4/5** | |
| Sync word | **0x22** | Private network, so it ignores other LoRa traffic |
| Preamble | **8** symbols | |
| Output power | **14 dBm** | Transmitter only, does not need to match |
| TCXO voltage | **1.6 V** | SX1262 only, see below |

**TCXO:** most SX1262 modules use a temperature-compensated crystal (TCXO) that the radio powers itself. RadioLib must know its voltage **before** `begin()`, so the code sets `radio.tcxoVoltage` first. If your module has a plain crystal (XTAL), set `LORA_TCXO_VOLTAGE` to `0`. If this value is wrong, `begin()` usually fails.

## Pins

| SX1262 signal | XIAO pin | Notes |
|---|---|---|
| **SCK** | D8 | Default XIAO SPI pins |
| **MISO** | D9 | |
| **MOSI** | D10 | |
| **NSS** (chip select) | D4 | |
| **DIO1** (packet done interrupt) | D1 | SX126x uses DIO1, not DIO0 |
| **RESET** | D2 | |
| **BUSY** | D3 | SX126x only: radio is busy, wait before the next command |
| **RF switch** | D5 | Switches the antenna between transmit and receive |

| LED | Pin |
|---|---|
| Red | 11 |
| Blue | 12 |
| Green | 13 |

RadioLib uses the default `SPI` object, so `SCK_PIN`, `MISO_PIN` and `MOSI_PIN` in [include/config.h](include/config.h) are only for reference. Pin assignments are defined in [include/config.h](include/config.h).

> **Always connect the antenna before you power the board.** The SX1262 can be damaged if it transmits without an antenna.

## Code structure

| Path | What it does |
|---|---|
| [src/main.cpp](src/main.cpp) | `setup()`: radio init. `loop()`: send a packet every 30 s and finish it when done |
| [include/config.h](include/config.h) | Pin assignments (radio and LEDs) |
| [include/lora.h](include/lora.h) | LoRa radio settings (must match the receiver) and the TCXO voltage |

## Build and run

```sh
pio run -t upload
pio device monitor
```

The board uses the Seeed platform from GitHub (see [platformio.ini](platformio.ini)), so the first build downloads it and takes longer.

The serial monitor runs at 115200 baud. With `LOG_LEVEL_VERBOSE` each packet looks like this:

```
[SX1262] Sending packet #3...
[SX1262]   transmission finished!
```

To see the packets arrive, flash and start the receiver from [05-ttgo-lora-receiver](../05-ttgo-lora-receiver).
