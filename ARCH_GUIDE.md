# Firmware architecture guide

A summary of the lecture on firmware architecture, kept **as it was taught**, without the decisions made for a project. It is the reference for a review: how far the real code went from the guide, and why. Each project keeps its own decisions, for example [08-wifi-captive-portal/HOW_IT_WORKS.md](08-wifi-captive-portal/HOW_IT_WORKS.md#architecture).

The examples in the lecture come from an acoustic sensor node: a MEMS microphone, a LoRa radio, a button, a LED and a battery.

## 1. Three layers and one rule

The rule is short: **dependencies only go down.** Everything else in the firmware architecture follows from this line.

```
App          modes, scenarios, decisions
  ↓
Services     detector · link · telemetry
  ↓
Drivers/HAL  mic_adc · radio_sx · nvs · btn
```

Never up. **The driver does not know the product, the app does not know the registers.**

| Layer | Knows | Does not know |
|---|---|---|
| **Driver** | registers and pins | the word "alarm". Replacing the microphone chip must not touch anything above |
| **Service** | the domain: dB, threshold, packet, retry | which ADC and which pin |
| **App** | the product: modes, scenarios, what to do on low battery | registers, bytes on the air |

## 2. A driver contract is an interface

C has no `interface` keyword, but a struct of function pointers does exactly the same. This is dependency injection, only without the words "dependency injection".

```c
// drivers/mic.h — the contract. There can be any number of implementations
typedef struct {
    bool     (*init)(void);
    bool     (*read)(uint16_t *raw);    // one sample
    void     (*sleep)(void);
    uint32_t  sample_hz;
} mic_drv_t;

// drivers/mic_mems.c — one of the implementations
static bool mems_read(uint16_t *raw) { *raw = adc1_get_raw(CH4); return true; }
const mic_drv_t mic_mems = { mems_init, mems_read, mems_sleep, 16000 };

// services/detector.c — the service does not know which microphone it has
void detector_init(const mic_drv_t *m) { s_mic = m; s_mic->init(); }
```

The cost is one extra pointer jump, a few nanoseconds. The gain is a fake driver and tests without the board.

## 3. The file tree is the architecture

If the layers cannot be seen in the tree, they do not exist.

```
firmware/
├── main/
│   └── app_main.c          entry point: create the queues and tasks
├── app/
│   ├── app_task.c          the only owner of the state, switch on events
│   └── modes.c             normal · alarm · economy · service
├── services/
│   ├── detector.c          RMS → dB → threshold → N-of-M → event
│   ├── link.c              packet, ACK, retry, backoff, LBT
│   ├── telemetry.c         charge, counters, node health
│   └── config.c            NVS: defaults, factory, field
├── drivers/
│   └── mic_mems.c  radio_sx.c  btn.c  led.c  nvs_store.c
├── proto/
│   └── packet.c  packet.h  the same file as in the gateway
└── test/
    └── test_detector.c     host tests, without the board
```

- No file in `drivers/` knows the word "alarm". No file in `app/` knows the word "register". This can be checked with grep.
- `proto/` is shared with the gateway firmware: one file, two platforms, zero differences in the format.

## 4. Who owns the device state

The most common cause of bugs that cannot be reproduced is two places writing to the same field. **The device state has exactly one owner**, otherwise the truth splits in two.

```
Events (from the queue) ──writes──► DEVICE STATE ──► Radio  (reads only)
                                    owner: app_task ──► UI     (reads only)
Driver ──✗ writes directly──►        mode · level ·   ──► Log    (reads only)
                                    link · charge ·
                                    counters
```

- **One writer.** Only `app_task` changes the state, and only in reaction to an event from the queue. Everyone else reads a copy.
- **Why not a mutex.** A mutex lets everyone write, just in turn. The problem is not the race, it is that the logic gets smeared.
- **Familiar?** It is the same conclusion the frontend came to: one store, actions, a reducer. The reason is the same.

## 5. The event queue is the backbone

One message type, one queue, one consumer. Everything else in the firmware is implementation details.

```
Microphone · ISR ─┐                            ┌─► Radio TX
Button · ISR ─────┤                            │
Timer ────────────┼─► EVENT QUEUE ─► app_task ─┼─► LED · vibro
Radio RX ─────────┘   event_t · 12 bytes       │
                                one switch     └─► Log
                                on event type
```

- Everything that happened becomes the same kind of message.
- **A producer does not know the consumer.** That is why a new event source is added without any change to `app_task`.
- That is also why the whole chain can be reproduced in a test: feed a sequence of events, get the expected state.

## 6. One structure for every source

An event has a type, a time and one number. This lasts surprisingly long, and when it stops being enough, you notice at once.

```c
// proto/event.h
typedef enum { EV_LEVEL, EV_BUTTON, EV_TICK, EV_RX, EV_LOW_BATT } ev_type_t;

typedef struct {
    ev_type_t type;
    uint32_t  t_ms;      // when it happened: monotonic time, not RTC
    int32_t   value;     // level in 0.1 dB · button code · RSSI
} event_t;               // 12 bytes, copied into the queue

// the button driver: everything it can and should do
void IRAM_ATTR btn_isr(void *arg) {
    event_t e = { EV_BUTTON, now_ms(), 0 };
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(q_events, &e, &woken);
    if (woken) portYIELD_FROM_ISR();
}
```

The queue copies the structure, so the ISR can hand the event over and forget it. No pointers to the stack, no `malloc` in an interrupt.
