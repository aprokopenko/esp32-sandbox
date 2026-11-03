# SPEC: button → radio over FreeRTOS tasks

Draft design for the [FreeRTOS basics](README.md) homework. The values marked _TBD_ are decided during implementation.

## Event flow

```
[button] ─(GPIO IRQ)─> Button ISR ──Q1(µs, level)──> Button Task
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
| **Button ISR** | puts `esp_timer_get_time()` and the pin level into Q1 | — | — |
| **Button Task** | debounce, single/double detection | Q1: forever when idle, until the deadline in the double-click window | medium |
| **Main loop** | app logic: click → radio command | Q2: `portMAX_DELAY` | low (Arduino default 1) |
| **Radio Task** | `startTransmit` → wait for DIO0 → `finishTransmit` | Q3 forever, then the notification with a timeout | high |
| **Radio ISR** | `vTaskNotifyGiveFromISR` to the radio task | — | — |
| **Display Task** | owns the counters and UI state, draws the OLED | Q4: forever | lowest |

No component polls flags or busy-waits. Every task sleeps on its own sync primitive.

## Queues

| Queue | Producer → consumer | Item | When full |
|---|---|---|---|
| **Q1** | Button ISR → Button Task | edge timestamp (µs) + pin level, 40 slots | drop, count in `btnIsrRejections` (no logs in an ISR) |
| **Q2** | Button Task → Main loop | click type + `t_press` (ms), 3 slots | wait up to 5 s, then drop and count in `btnEventRejections`. Error event to Q4: _TBD_ |
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
- Triggers on `CHANGE`, both edges: the task needs releases to tell release bounce from a new press.
- Reads the pin level with `digitalRead`. Inside a bounce burst this value is unreliable.
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
| CH1 | Debug 1 (GPIO 13) | button ISR entry |
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
