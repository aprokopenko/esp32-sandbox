# SPEC: button → radio over FreeRTOS tasks

Draft design for the [FreeRTOS basics](README.md) homework. The values marked _TBD_ are decided during implementation.

## Event flow

```
[button] ─(GPIO IRQ)─> Button ISR ──Q1(timestamp)──> Button Task
                                                        │ SINGLE/DOUBLE + t_press
                                                        ▼
                                               Q2 ──> Main loop (dispatcher)
                                                        │ radio command + t_press
                                                        ▼
                                               Q3 ──> Radio Task ──SPI──> SX1276
                                                        ▲                    │
                                                        └──notification── Radio ISR (DIO0)

Button Task / Main loop / Radio Task ──Q4(UI events)──> Display Task ──I2C──> OLED
```

## Components

| Component | Job | Waits on | Priority |
|---|---|---|---|
| **Button ISR** | puts `esp_timer_get_time()` into Q1 | — | — |
| **Button Task** | debounce, single/double detection | Q1: forever when idle, until the deadline in the double-click window | medium |
| **Main loop** | app logic: click → radio command | Q2: `portMAX_DELAY` | low (Arduino default 1) |
| **Radio Task** | `startTransmit` → wait for DIO0 → `finishTransmit` | Q3 forever, then the notification with a timeout | high |
| **Radio ISR** | `vTaskNotifyGiveFromISR` to the radio task | — | — |
| **Display Task** | owns the counters and UI state, draws the OLED | Q4: forever | lowest |

No component polls flags or busy-waits. Every task sleeps on its own sync primitive.

## Queues

| Queue | Producer → consumer | Item | When full |
|---|---|---|---|
| **Q1** | Button ISR → Button Task | press timestamp (µs) | drop silently (it's an ISR) |
| **Q2** | Button Task → Main loop | click type + `t_press` | drop, send an error event to Q4 |
| **Q3** | Main loop → Radio Task | radio command + `t_press` | drop, send an error event to Q4 |
| **Q4** | Button Task, Main loop, Radio Task → Display Task | UI event code (enum) + numbers | drop |

Rules:
- Queues **copy bytes**. Items are plain structs with fixed-size fields, never a `char*` to a local buffer.
- `t_press` travels with the event from the ISR to the radio task, for the latency measurement.
- Text for the OLED is formatted inside the display task, not by the producers.
- Every overflow is logged by the producer of that queue, so it's clear where the jam is.

## Button ISR

- Marked `IRAM_ATTR`.
- Calls only `...FromISR` API: `xQueueSendFromISR` with `xHigherPriorityTaskWoken` and `portYIELD_FROM_ISR()`. Without the yield the button task would only wake on the next tick.
- Does nothing else: no logic, no Serial, no logs.
- Contact bounce produces 5–20 interrupts per press. A failed send is fine: the event is simply dropped.

## Button Task

A small state machine with two states:

| State | Waits on Q1 | On event | On timeout |
|---|---|---|---|
| **IDLE** | forever | accept the press, set `deadline = t_press + window`, go to WAIT_SECOND | — |
| **WAIT_SECOND** | until `deadline` | bounce → ignore; real press → send `DOUBLE`, go to IDLE | send `SINGLE`, go to IDLE |

Details:
- **Debounce by timestamp:** an edge closer than _N_ ms to the last accepted one is bounce.
- **Release bounce:** releasing the button also bounces and produces falling edges. They can arrive 100–200 ms after the press, inside the double-click window, and must not count as a second click. Options: check the pin level, track both edges (full press → release cycles), or a longer debounce. _TBD._
- **Timeout from the deadline:** each wait in WAIT_SECOND uses the time left until `deadline`, it does not restart the full window after an ignored event.
- **Third click** and events after the decision: behavior is defined explicitly. _TBD._
- The double-click window is _TBD_ (typically 250–400 ms). It is a trade-off: a shorter window means a faster `SINGLE`, but a harder `DOUBLE`.
- Bring-up: debug the click detection first without the radio (the loop just prints the result).

## Main loop

- Blocks on Q2 with `portMAX_DELAY`, no `delay()`.
- Acts as the dispatcher: the button task does not know about the radio, the radio task does not know about the button. New event sources or app states only change the loop.
- If Q3 is full: send with timeout `0`, drop the command, log it and send an error event to Q4.

## Radio Task

```
forever:
    wait command from Q3 (forever)
    clear stale notification
    startTransmit(packet)
    wait notification (timeout = time on air + margin)
    timeout → log error
    finishTransmit()
    send "TX done" event to Q4
```

- **Owner of the radio:** only this task calls `radio.*`. RadioLib and the SPI bus are not thread-safe.
- `startTransmit` is asynchronous: it loads the packet over SPI and returns. During time on air the task is blocked and uses no CPU, so a high priority does not starve the other tasks.
- **TX done** comes from the SX1276 **DIO0** pin through the RadioLib packet-sent callback, which is an ISR.
- **Notification instead of a flag:** the ISR wakes the task with `vTaskNotifyGiveFromISR`. A task notification is a lighter binary/counting semaphore bound to one task.
- **Clicks during a transmission** wait in Q3. The task does not read Q3 while it waits for DIO0, so the two channels never mix.
- **Stale notification:** if a wait times out and DIO0 fires later, the notification stays pending, and the next wait would return at once. The state is cleared before every `startTransmit`.
- **Timeout is mandatory:** without it, a lost interrupt or a hung chip blocks the task forever.

## Radio ISR

- Marked `IRAM_ATTR`.
- Only `vTaskNotifyGiveFromISR` + `portYIELD_FROM_ISR()`. No SPI, no RadioLib calls, no logs.

## Display Task

- **Owner of `Wire` and the display:** only this task touches the I2C bus and the SSD1306 object.
- **Owns the UI state:** click counters, sent counter, last error. Producers send events (enum + numbers), the task updates its own state and redraws. No shared variables, no mutex.
- A normal queue, not `xQueueOverwrite`: with several producers a mailbox would let a "TX done" overwrite an "error".
- A full frame is 1024 bytes ≈ 23 ms at 400 kHz, 90+ ms at 100 kHz. The clock is set explicitly with `Wire.setClock`.
- Lowest priority, so drawing never delays the button → radio path.

## Priorities and cores

| Task | Priority (draft) | Why |
|---|---|---|
| Radio Task | 5 | short bursts of work, blocked during time on air |
| Button Task | 3 | needs accurate timing for the double-click window |
| Main loop | 1 | Arduino default, short work per event |
| Display Task | 1 or lower than loop | slow I2C, not time-critical |

- Tasks are pinned to cores explicitly with `xTaskCreatePinnedToCore`, so the behavior is predictable. ESP32 has two cores, and unpinned tasks can hide priority problems. Core layout: _TBD_.
- The reasons for each priority are written as comments next to `xTaskCreate...`.
- Stack sizes are checked with `uxTaskGetStackHighWaterMark`.

## Latency measurement ⭐

### Logic analyzer channels

| Channel | Signal | Marks |
|---|---|---|
| CH0 | button line (GPIO 15) | physical press, bounce |
| CH1 | Debug 1 (GPIO 13) | _TBD_, e.g. click decided / command put into Q3 |
| CH2 | Debug 2 (GPIO 14) | _TBD_, e.g. `startTransmit` called |
| CH3 | DIO0 (GPIO 26) | physical end of the transmission |

### Software

`t_press` from the button ISR is carried in the queue items down to the radio task, which computes the delta with `esp_timer_get_time()`. Logs are printed **after** the measurement, never in the hot path, because `Serial` changes the timing.

### Expected components of the delay

1. ISR entry: µs.
2. **Double-click window for `SINGLE`: hundreds of ms**, by design of the algorithm. `DOUBLE` does not wait for it.
3. Task switches through the queues: µs, if the priorities are right.
4. SPI packet load: < 1 ms.
5. **Time on air: tens to hundreds of ms**, depends on SF, BW and packet size.

### Control experiment

Measure with and without the display task. If the priorities are right, the button → radio latency barely changes.

### Definition of "sent"

The report states which moment counts as "the message was sent": the `startTransmit` call or the DIO0 edge. The difference between them is the time on air.

Results: _TBD_.
