# TTGO LoRa32: custom SPI

A TTGO LoRa32 example that talks to the onboard **SX1276** LoRa radio over SPI **without any SPI library**. The SPI protocol is implemented by hand (bit-banging): every clock edge and every data bit is driven with `digitalWrite()` and read with `digitalRead()`.

## The task

> Implement the simplest data exchange between the dev board and the radio chip over SPI. The main option is a TTGO LoRa32 board with the built-in SX1276 radio chip. Another board or SPI device can be used if its pinout and datasheet are available.
>
> **Main rule:** do not use ready-made functions from `SPI.h`, RadioLib or other libraries to perform the SPI exchange itself.

## What is done

- **[lib/CustomSPI](lib/CustomSPI):** own SPI master class. It supports SPI **mode 0** (clock `LOW` while idle, data read on the rising edge) with **MSB first**, which is the mode the SX1276 uses.
- **Register read:** `readRegister(addr)` reads one SX1276 register.
- **Check:** on startup the firmware reads `RegVersion` (`0x42`). The SX1276 always returns **`0x12`** there, so getting `0x12` proves that the clock, both data lines and chip select all work.

Besides the radio, `setup()` also scans the I2C bus and prints "Hello world!" on the onboard SSD1306 OLED.

## Pins

The radio is wired to the ESP32 on the board, so no external wiring is needed.

| SX1276 signal | Direction | ESP32 pin | Pin mode |
|---|---|---|---|
| **SCK** (clock) | ESP32 → radio | **GPIO 5** | `OUTPUT`, `LOW` while idle |
| **MOSI** (data to radio) | ESP32 → radio | **GPIO 27** | `OUTPUT` |
| **MISO** (data from radio) | radio → ESP32 | **GPIO 19** | `INPUT_PULLUP` |
| **NSS** (chip select) | ESP32 → radio | **GPIO 18** | `OUTPUT`, `HIGH` while idle |

`MISO` uses the internal pull-up, so a radio that doesn't answer reads as `0xFF`, not random noise. This makes "the chip is silent" easy to tell apart from "the chip answered with wrong data".

Pin assignments are defined in [include/pins.h](include/pins.h).

## Code structure

| Path | What it does |
|---|---|
| [src/main.cpp](src/main.cpp) | `setup()`: I2C scan, OLED "Hello world!", SPI init and `RegVersion` read |
| [lib/CustomSPI](lib/CustomSPI) | Bit-banged SPI master: `begin()`, `readRegister(addr)`, private `transfer(byte)` |
| [include/pins.h](include/pins.h) | Pin assignments and the OLED I2C address |
| [include/helpers.h](include/helpers.h) | `byteToHex()` for printing bytes as `0x..` with a leading zero |

## Build and run

```sh
pio run -t upload
pio device monitor
```

The serial monitor runs at 115200 baud. A successful run ends with:

```
Reading radio version...
- radio version: 0x12
```

With `LOG_LEVEL_VERBOSE` (set in [platformio.ini](platformio.ini)) the log also shows every transferred bit, `out` on `MOSI` and `in` on `MISO`.
