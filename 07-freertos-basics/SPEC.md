# SPEC: button → radio over FreeRTOS tasks

Design for the [FreeRTOS basics](README.md) homework. The values marked _TBD_ are decided during implementation.

**Status:** all components are implemented. Passing `t_press` to the radio task, the software latency measurement and the control experiment are planned.

## Event flow

```
[button] ─(GPIO IRQ)─> Button ISR ──Q1(µs, level)──> Button Task
                                                        │ SINGLE/DOUBLE + t_press
                                                        ▼
                                               Q2 ──> Main loop (dispatcher)
                                                        │ click count
                                                        ▼
                                               Q3 ──> Radio Task ──SPI──> SX1276
                                                        ▲                    │
                                                        └──semaphore──── Radio ISR (DIO0)

Main loop / Radio Task ──display::post()──Q4(UI events)──> Display Task ──I2C──> OLED
```

## Components

| Component | Job | Waits on | Priority |
|---|---|---|---|
| **Button ISR** | puts `esp_timer_get_time()` and the pin level into Q1 | — | — |
| **Button Task** | debounce, single/double detection | Q1: forever when idle, until the deadline in the double-click window | medium |
| **Main loop** | app logic: click → radio command, click → UI event | Q2: `portMAX_DELAY` | lowest (Arduino default 1) |
| **Radio Task** | `startTransmit` → wait for DIO0 → `finishTransmit` → UI event | Q3 forever, then the TX semaphore with a timeout | high |
| **Radio ISR** | `xSemaphoreGiveFromISR` to the radio task | — | — |
| **Display Task** | owns the I2C bus, the OLED and the UI counters, draws the screen | Q4: forever | low |

No component polls flags or busy-waits. Every task sleeps on its own sync primitive.

## Queues

| Queue | Producer → consumer | Item | When full |
|---|---|---|---|
| **Q1** | Button ISR → Button Task | edge timestamp (µs) + pin level, 40 slots | drop, count in `btnIsrRejections` (no logs in an ISR) |
| **Q2** | Button Task → Main loop | click type + `t_press` (ms), 3 slots | wait up to 5 s, then drop, count in `btnEventRejections` and log. Error event to Q4: _TBD_ |
| **Q3** | Main loop → Radio Task | click count (`unsigned int`), 10 slots | wait up to 1 s, then drop and log. Planned: timeout `0` and an error event to Q4 |
| **Q4** | Main loop, Radio Task → Display Task | `UiMessage`: `UiEvent` enum + `int32_t` value, 10 slots | timeout `0`, drop and count in `uiDrops` (shown on the OLED) |

Rules:
- Queues **copy bytes**. Items are plain structs with fixed-size fields, never a `char*` to a local buffer.
- `t_press` travels with the event from the ISR to the radio task, for the latency measurement. Now it reaches the main loop only; Q3 carries the click count.
- Text for the OLED is formatted inside the display task, not by the producers.
- Every overflow is logged by the producer of that queue, so it's clear where the jam is. Exception: Q4 overflows are only counted, the UI is not worth a log line in a producer's path.
- Q4 is private to the display module. Producers call `display::post()`, which hides the queue, the timeout `0` and the drop counter.

## Button ISR

- Marked `IRAM_ATTR`.
- Calls only `...FromISR` API: `xQueueSendFromISR` with `xHigherPriorityTaskWoken` and `portYIELD_FROM_ISR()`. Without the yield the button task would only wake on the next tick.
- Triggers on `CHANGE`, both edges: the task needs releases to tell release bounce from a new press.
- Reads the pin level with `digitalRead`. Inside a bounce burst this value is unreliable.
- Sets the Debug 1 pin HIGH with `gpio_set_level` (see [Logic analyzer channels](#logic-analyzer-channels)).
- Does nothing else: no logic, no Serial, no logs.
- Contact bounce produces 5–20 interrupts per press and again per release. If Q1 is full, the edge is dropped and `btnIsrRejections` grows.

## Button Task

Two layers in one task. Layer 1 turns raw edges into Press / Release, layer 2 turns presses into `SINGLE` / `DOUBLE`. Releases only matter for layer 1.

Constants: debounce 30 ms, double-click window 350 ms.

### Layer 1: debounce (simplified lockout)

| Edge | Result |
|---|---|
| less than 30 ms after the previous edge | `Glitch`, ignored. The window slides: every edge, bounce included, moves it |
| 30 ms or more after the previous edge, level HIGH → LOW | `Press` |
| 30 ms or more after the previous edge, level LOW → HIGH | `Release` |
| 30 ms or more after the previous edge, same level | `Undefined`, ignored |

The level comparison uses the level of the previous edge and the level the ISR read for this edge. A press is announced on the first edge of the burst, so debounce adds no latency.

### Layer 2: click state machine

| State | Waits on Q1 | Press, `t ≤ deadline` | Press, `t > deadline` | Deadline passed |
|---|---|---|---|---|
| **Idle** | forever | start: `deadline = t_press + 350 ms` → Processing | same | — |
| **Processing** | until `deadline` | send `DOUBLE` → Idle | send `SINGLE`, put the edge back to the front of Q1, it starts a new click → Idle | send `SINGLE` → Idle |

Details:
- **Decisions by timestamps.** When an edge arrives, its ISR timestamp is compared with `deadline`, on a timeout `now` is. A late task (e.g. the CPU was busy) still gets the right answer: a press queued before the deadline still becomes `DOUBLE`.
- **Timeout from the deadline:** each wait uses the time left until `deadline`, converted to ticks with rounding up and at least 1 tick. It does not restart the full window after an ignored edge.
- **Release bounce** is filtered by layer 1, so it can't start a new click after `DOUBLE`.
- **Third click:** after `DOUBLE` the machine is Idle, so a triple click gives `DOUBLE` + `SINGLE`.
- A re-queued edge is recognized by its timestamp and is not classified by layer 1 a second time.
- After sending the event to Q2 the task sets Debug 1 LOW.

### Known limitations

Layer 1 is not a full lockout algorithm. A full one takes the direction from the stable (announced) state, not from the ISR reading, and checks the level of the last edge when the line goes quiet. Without it a click can be lost in rare cases:
- **The ISR reads the first edge after it bounced back.** HIGH → HIGH gives `Undefined`, the rest of the burst is `Glitch`, and the press is lost. Nothing corrects it later.
- **Very fast double click:** if the second press comes less than 30 ms after the release bounce, it falls into the sliding window and becomes `Glitch`. The result is `SINGLE` instead of `DOUBLE`.
- **Noise spike** after a quiet period is taken as a press.
- **Re-queue to Q1** blocks the task for up to 1 s if Q1 is full.

Planned fix: direction from the stable state, a quiet check (by the next edge's timestamp or by a queue timeout) that compares the last edge's level with the announced one and emits a correcting or late event.

### Diagnostics

`btnIsrRejections` and `btnEventRejections` are kept in `RTC_NOINIT_ATTR` memory with a magic value, so they survive a software or watchdog reset and show whether queues overflowed before a hang.

## Main loop

- Blocks on Q2 with `portMAX_DELAY`, no `delay()`.
- Acts as the dispatcher: the button task does not know about the radio, the radio task does not know about the button. New event sources or app states only change the loop.
- Posts `ClickSingle` / `ClickDouble` to the display, then maps `SINGLE` → 1 click, `DOUBLE` → 2 clicks and calls `radio::sendClicksNotification`, but only if `radio::isAvailable` (the radio initialized). Clicks are counted on the OLED even without a radio.
- If Q3 is full: waits up to 1 s, drops the command and logs it. Planned: timeout `0` and an error event to Q4, so the loop never stalls.

## Radio Task

```
forever:
    wait click count from Q3 (forever)
    clear stale semaphore (take with timeout 0)
    startTransmit(message)
    error → log, post TxFailed(code), skip to the next command
    Debug 2 HIGH
    wait semaphore (timeout = time on air + 20 ms)
    timeout → state = RADIOLIB_ERR_TX_TIMEOUT
    log the result (done or error code)
    finishTransmit()
    Debug 2 LOW
    post TxDone or TxFailed(code) to the display
    log the stack high water mark if it reached a new minimum
```

Messages:

| Click count | Radio packet |
|---|---|
| 1 | `SINGLE CLICK` |
| 2 | `DOUBLE CLICK` |
| anything else | `MANY CLICKS` |

- **Owner of the radio:** only this task calls `radio.*` after `begin()`. RadioLib and the SPI bus are not thread-safe.
- `startTransmit` is asynchronous: it loads the packet over SPI and returns. During time on air the task is blocked and uses no CPU, so a high priority does not starve the other tasks.
- **TX done** comes from the SX1276 **DIO0** pin through the RadioLib packet-sent callback (`setPacketSentAction`), which is an ISR.
- **Binary semaphore instead of a flag:** the ISR wakes the task with `xSemaphoreGiveFromISR`. A task notification (`vTaskNotifyGiveFromISR`) would also work and is lighter, but needs the task handle.
- **Clicks during a transmission** wait in Q3. The task does not read Q3 while it waits for DIO0, so the two channels never mix.
- **Stale signal:** if a wait times out and DIO0 fires later, the semaphore stays given, and the next wait would return at once. It is cleared before every `startTransmit`.
- **Timeout is mandatory:** without it, a lost interrupt or a hung chip blocks the task forever. The timeout comes from `getTimeOnAir(len)` (µs), converted to ms, plus a 20 ms margin.
- **`finishTransmit()` runs after a timeout too:** it puts the chip into standby and clears the IRQ flags.
- **Init order** in `radio::begin()`: chip → semaphore → Q3 → task → `setPacketSentAction` → `isAvailable = true`. A resource is created before anything that uses it.
- **UI events after Debug 2 LOW:** posting to the display is not part of the measured path.

Known gap: if `startTransmit` fails, the task skips `finishTransmit()` and leaves Debug 2 untouched.

## Radio ISR

- Marked `IRAM_ATTR`.
- Only `xSemaphoreGiveFromISR` + `portYIELD_FROM_ISR()`. No SPI, no RadioLib calls, no logs.

## Display Task

```
init: Wire.begin(SDA, SCL), Wire.setClock(400 kHz), I2C scan, oled.begin, draw zeros
forever:
    wait event from Q4 (forever)
    apply it to the UI state
    drain Q4 with timeout 0, apply every event
    redraw once
    log the stack high water mark if it reached a new minimum
```

Screen:

```
Button -> LoRa

Single:  <n>
Double:  <n>
Sent:    <n>
Failed:  <n> (<last error code>)
UI drops: <n>
```

- **Owner of `Wire` and the display:** only this task touches the I2C bus and the SSD1306 object. The I2C and OLED init runs inside the task, so `setup()` is not blocked by I2C.
- **Owns the UI state:** click counters, sent counter, failed counter, last error. Producers send events (enum + number), the task updates its own state and redraws. No shared variables, no mutex. The only exception is `uiDrops`: producers increment it, so it is a `std::atomic`.
- **One redraw per burst:** after the first event the task drains the queue without waiting, so e.g. `ClickDouble` + `TxDone` cost one frame, not two.
- A normal queue, not `xQueueOverwrite`: with several producers a mailbox would let a "TX done" overwrite an "error".
- A full frame is 1024 bytes ≈ 23 ms at 400 kHz, 90+ ms at 100 kHz. The clock is set explicitly with `Wire.setClock`.
- If `oled.begin` fails, the task logs an error and keeps draining Q4 without drawing, so `display::post()` behaves the same.
- Text is formatted here with `printf`, using `PRIu32` / `PRId32` for the fixed-width counters.

## Priorities and cores

| Task | Priority | Core | Stack | Why |
|---|---|---|---|---|
| Radio Task | 5 | 0 | 3072 | short bursts of work, blocked during time on air |
| Button Task | 3 | any (`tskNO_AFFINITY`) | 3072 | needs accurate timing for the double-click window |
| Display Task | 2 | 1 | 3072 | slow I2C, not time-critical: below the button and radio tasks |
| Main loop | 1 | 1 (Arduino) | Arduino default (8192) | short work per event |

- Tasks are pinned to cores explicitly with `xTaskCreatePinnedToCore`, so the behavior is predictable. ESP32 has two cores, and unpinned tasks can hide priority problems. The button task is still unpinned: _TBD_.
- The reasons for each priority are written as comments next to `xTaskCreate...`.
- **Stack sizes** (in bytes on ESP-IDF) are measured with `uxTaskGetStackHighWaterMark(nullptr)`. Each task logs the value whenever it reaches a new minimum, after the work of the iteration. The sizes are the measured use plus a margin. Not every error path was exercised (e.g. the radio timeout), the margin covers them. The main loop stack is not measured.

## Latency measurement ⭐

### Logic analyzer channels

| Channel | Signal | Marks |
|---|---|---|
| CH0 | button line (GPIO 15) | physical press, bounce |
| CH1 | Debug 1 (GPIO 13) | HIGH: set by the button ISR on every edge. LOW: the button task sent the click to Q2. The falling edge is the moment the click is classified |
| CH2 | Debug 2 (GPIO 14) | HIGH: `startTransmit` returned OK. LOW: `finishTransmit` done |
| CH3 | DIO0 (GPIO 26) | physical end of the transmission. Stays HIGH until `finishTransmit` clears the IRQ flags |

Debug 1 is not a clean "ISR entry" mark: a release edge also sets it HIGH, and it stays HIGH until the next click is sent. A short pulse (HIGH at ISR entry, LOW at exit) would mark every edge. _TBD_.

### Software

`t_press` from the button ISR is carried in the queue items down to the radio task, which computes the delta with `esp_timer_get_time()`. Logs are printed **after** the measurement, never in the hot path, because `Serial` changes the timing. Now `t_press` reaches the main loop only: _TBD_.

### Expected components of the delay

1. ISR entry: µs.
2. **Double-click window for `SINGLE`: hundreds of ms**, by design of the algorithm. `DOUBLE` does not wait for it.
3. Task switches through the queues: µs, if the priorities are right.
4. SPI packet load: < 1 ms.
5. **Time on air: tens to hundreds of ms**, depends on SF, BW and packet size.

### Control experiment

Measure with and without the display task. If the priorities are right, the button → radio latency barely changes. _TBD_.

### Definition of "sent"

The report states which moment counts as "the message was sent": the `startTransmit` call or the DIO0 edge. The difference between them is the time on air.

### Results

First logic analyzer capture, before the display task existed. SF7, BW 250 kHz, CR 4/5, preamble 8, 12-byte packet:

| Interval | Measured | Expected |
|---|---|---|
| Press → click sent to Q2, `SINGLE` (CH0 ↓ → CH1 ↓) | ≈ 350 ms | 350 ms double-click window |
| Second press → click sent to Q2, `DOUBLE` | ≈ 0 ms, no wait for the window | — |
| Click sent → radio starts (CH1 ↓ → CH2 ↑) | a few ms | _TBD_, includes a `log_d` before `startTransmit` |
| Time on air (CH2 ↑ → CH3 ↑) | ≈ 21 ms | 20.6 ms by the LoRa formula |

Software measurement and the control experiment: _TBD_.
