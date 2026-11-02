#include "radio.h"
/*
SX1262 radio = new Module(CS0_PIN, DIO1_PIN, NRST_PIN, BUSY_PIN);

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

void setFlag(void) {
  // we sent a packet, set the flag
  transmittedFlag = true;
}

void radioBegin() {

  // before calling begin(), correct crystal has to be selected
  // most SX126x have a TCXO which needs 1.6V reference
  // set to 0 if your radio has an XTAL
  radio.tcxoVoltage = LORA_TCXO_VOLTAGE;

  Log.infoln("[SX1262] Initializing LoRa...");
  ConfigLoRa_t config;
  config.frequency       = LORA_FREQUENCY_MHZ;
  config.bandwidth       = LORA_BANDWIDTH_KHZ;
  config.spreadingFactor = LORA_SPREADING_FACTOR;
  config.codingRate      = LORA_CODING_RATE;
  config.syncWord        = LORA_SYNC_WORD;
  config.power           = LORA_POWER_DBM;
  config.preambleLength  = LORA_PREAMBLE_LENGTH;

  int state = radio.begin(config);

  if (state == RADIOLIB_ERR_NONE) {
    Log.infoln("   success!");

    radio.setRfSwitchPins(RF_SWITCH_PIN, RADIOLIB_NC);
  } else {
    Log.errorln("   failed, code %d", state);
    while (true) { delay(10); } // block processing
  }

  // set the function that will be called
  // when packet transmission is finished
  radio.setPacketSentAction(setFlag);

  pinMode(LEDG_PIN, OUTPUT);
  pinMode(LEDR_PIN, OUTPUT);
  digitalWrite(LEDG_PIN, LOW);
}

// counter to keep track of transmitted packets
unsigned int packagesSent = 0;
uint32_t lastTransmissionMs = 0;
uint64_t lastTransmissionUs = 0;
constexpr uint16_t TRANSMISSIONS_DELAY_MS = 15000;

void radioSend() {
  // check if the previous transmission finished
  if(transmittedFlag) {
    // reset flag
    transmittedFlag = false;

    uint64_t txTimeUs = micros() - lastTransmissionUs;

    if (transmissionState == RADIOLIB_ERR_NONE) {
      Log.traceln("[SX1262]   transmission finished in %uus!", txTimeUs);
    } else {
      Log.errorln("[SX1262]   transmission failed! Code %d", transmissionState);
    }

    // clean up after transmission is finished
    // this will ensure transmitter is disabled,
    // RF switch is powered down etc.
    radio.finishTransmit();
    digitalWrite(LEDR_PIN, HIGH);
  }

  uint32_t nowMs = millis();
  if (
    (!lastTransmissionMs || (nowMs - lastTransmissionMs >= TRANSMISSIONS_DELAY_MS)) 
    && !transmittedFlag
  ) {
    Log.traceln("[SX1262] Sending packet #%u...", ++packagesSent);

    float cpuTemp = readCPUTemperature();

    // you can transmit C-string string up to 256 characters long
    uint8_t msg[LORA_FIXED_PAYLOAD_SIZE ? LORA_FIXED_PAYLOAD_SIZE : 64];
    if (LORA_FIXED_PAYLOAD_SIZE) {
      memset(msg, 0xAA, sizeof(msg));
    }
    snprintf((char*)msg, sizeof(msg), "Package #%u at %lus, T=%.1f", packagesSent, nowMs / 1000, cpuTemp);
    size_t len = LORA_FIXED_PAYLOAD_SIZE ? sizeof(msg) : strlen((char*)msg);
    Log.traceln("[SX1262]    message (%dB): %s", len, (char*)msg);

    Log.traceln("[SX1262]    expected time on air: %uus", (uint32_t)radio.getTimeOnAir(len));

    transmissionState = radio.startTransmit(msg, len);

    lastTransmissionMs = nowMs;
    lastTransmissionUs = micros();
    digitalWrite(LEDR_PIN, LOW);
  }
}
*/