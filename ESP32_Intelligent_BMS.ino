/*
  ESP32 INTELLIGENT BMS - FINAL INTEGRATED DEMONSTRATION SKETCH
  Board: ESP32 DevKit V1 in Wokwi

  PIN MAP
  Cell potentiometers: GPIO34, GPIO35, GPIO32, GPIO33
  I2C LCD 16x2: SDA GPIO21, SCL GPIO22, address 0x27
  Relay IN: GPIO25
  LED through 220 ohm resistor: GPIO26
  Push button: GPIO27 to GND (INPUT_PULLUP)

  Wokwi libraries:
    - Blynk
    - LiquidCrystal I2C

  SECURITY:
  Replace BLYNK_AUTH_TOKEN with a newly regenerated device token.
  The token previously posted in chat should be revoked/regenerated.
*/

#define BLYNK_TEMPLATE_ID   "TMPL3ObNFPWPS"
#define BLYNK_TEMPLATE_NAME "ESP32 Intelligent BMS"
#define BLYNK_AUTH_TOKEN    "PASTE_NEW_DEVICE_AUTH_TOKEN_HERE"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/* ================= CONFIGURATION ================= */

#define NUM_CELLS 4
#define RELAY_PIN 25
#define BUTTON_PIN 27
#define LED_PIN 26
#define I2C_SDA 21
#define I2C_SCL 22
#define LCD_ADDR 0x27

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASS ""

#define ADC_MAX 4095UL
#define CELL_MAX_MV 4200UL
#define UV_LIMIT_MV 3000
#define OV_LIMIT_MV 4250

#define BASE_IMBALANCE_LIMIT_MV 100
#define MIN_IMBALANCE_LIMIT_MV 60
#define MAX_IMBALANCE_LIMIT_MV 120

#define QSIZE 8
#define SENSOR_INTERVAL_MS 200UL
#define LCD_REFRESH_MS 250UL
#define LCD_PAGE_MS 3000UL
#define TELEMETRY_INTERVAL_MS 3000UL
#define WIFI_CHECK_MS 2000UL
#define ANALYTICS_INTERVAL_MS 5000UL
#define RECOVERY_VERIFY_MS 3000UL
#define BUTTON_DEBOUNCE_MS 40UL
#define RELAY_ON_CONFIRM_MS 500UL
#define CELL_EVENT_DELTA_MV 20
#define IMBALANCE_EVENT_DELTA_MV 20

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
const uint8_t cellPins[NUM_CELLS] = {34, 35, 32, 33};

/* ================= ENUMS / STRUCTURES ================= */

enum ImbalanceTrend : uint8_t {
  TREND_STABLE,
  TREND_INCREASING,
  TREND_DECREASING
};

enum SystemState : uint8_t {
  NORMAL,
  DEGRADED,
  FAILSAFE,
  SHUTDOWN
};

enum FaultId : uint8_t {
  FAULT_NONE,
  FAULT_CELL_VOLTAGE,
  FAULT_IMBALANCE,
  FAULT_ADC_RANGE,
  FAULT_VOLTAGE_JUMP,
  FAULT_ADC_FROZEN,
  FAULT_COMMUNICATION,
  FAULT_MANUAL_SHUTDOWN
};

struct TelemetryEvent {
  uint8_t type;       // 1 cell change, 2 imbalance, 3 state, 4 fault, 5 relay
  uint8_t id;
  int32_t oldValue;
  int32_t newValue;
  uint32_t timestamp;
};

/* ================= BMS DATA ================= */

uint16_t cellV[NUM_CELLS] = {0};
uint16_t filteredV[NUM_CELLS] = {0};
uint16_t previousV[NUM_CELLS] = {0};
uint16_t rawNow[NUM_CELLS] = {0};
uint16_t lastRaw[NUM_CELLS] = {0};
uint8_t sameRawCount[NUM_CELLS] = {0};

uint8_t weakCell = 0;
uint8_t strongCell = 0;
uint16_t imbalance = 0;
uint16_t previousImbalance = 0;
uint16_t adaptiveImbalanceLimit = BASE_IMBALANCE_LIMIT_MV;
uint8_t estimatedSOC = 0;
ImbalanceTrend imbalanceTrend = TREND_STABLE;

bool adcFault = false;
bool cellFault = false;
bool commFault = false;
bool manualFault = false;
bool frozenADC = false;
bool voltageJump = false;

SystemState state = NORMAL;
SystemState previousState = NORMAL;
FaultId activeFault = FAULT_NONE;

bool relayState = false;
bool previousRelayState = false;
bool relayCommand = false;
bool wifiOnline = false;
bool telemetryOnline = false;
int wifiRSSI = -100;

float riskScore = 0.0f;
uint32_t totalFaults = 0;
uint32_t uptimeStart = 0;

TelemetryEvent eventQueue[QSIZE];
uint8_t qHead = 0, qTail = 0, qCount = 0;

uint32_t lastSensor = 0;
uint32_t lastLCD = 0;
uint32_t lastPage = 0;
uint32_t lastTelemetry = 0;
uint32_t lastWiFi = 0;
uint32_t lastAnalytics = 0;
uint32_t lastRelayCommandChange = 0;
uint32_t faultClearSince = 0;
uint32_t lastButtonChange = 0;
uint32_t lastReplay = 0;

uint8_t lcdPage = 0;
bool recoveryPending = false;
bool lastButtonReading = HIGH;
bool stableButtonReading = HIGH;
bool faultWasActive = false;
char lcdLast[2][17] = {"", ""};

/* ================= TEXT HELPERS ================= */

const char* stateName(SystemState s) {
  switch (s) {
    case NORMAL: return "NORMAL";
    case DEGRADED: return "DEGRADED";
    case FAILSAFE: return "FAILSAFE";
    case SHUTDOWN: return "SHUTDOWN";
    default: return "UNKNOWN";
  }
}

const char* trendName(ImbalanceTrend t) {
  switch (t) {
    case TREND_INCREASING: return "INCREASING";
    case TREND_DECREASING: return "DECREASING";
    default: return "STABLE";
  }
}

const char* faultName(FaultId f) {
  switch (f) {
    case FAULT_CELL_VOLTAGE: return "CELL_VOLTAGE";
    case FAULT_IMBALANCE: return "IMBALANCE";
    case FAULT_ADC_RANGE: return "ADC_RANGE";
    case FAULT_VOLTAGE_JUMP: return "VOLTAGE_JUMP";
    case FAULT_ADC_FROZEN: return "ADC_FROZEN";
    case FAULT_COMMUNICATION: return "COMMUNICATION";
    case FAULT_MANUAL_SHUTDOWN: return "MANUAL_SHUTDOWN";
    default: return "NONE";
  }
}

/* ================= FIXED-SIZE EVENT QUEUE ================= */

bool enqueueEvent(uint8_t type, uint8_t id, int32_t oldValue, int32_t newValue) {
  // Coalesce only sensor-change events; preserve state/fault/relay transition order.
  if (type == 1 || type == 2) {
    uint8_t p = qHead;
    for (uint8_t i = 0; i < qCount; ++i) {
      if (eventQueue[p].type == type && eventQueue[p].id == id) {
        eventQueue[p].newValue = newValue;
        eventQueue[p].timestamp = millis();
        return true;
      }
      p = (p + 1) % QSIZE;
    }
  }

  if (qCount >= QSIZE) {
    // Prefer dropping the oldest low-priority measurement event so state/fault/
    // relay transitions have a better chance of being retained.
    int8_t dropOffset = -1;
    for (uint8_t i = 0; i < qCount; ++i) {
      uint8_t idx = (qHead + i) % QSIZE;
      if (eventQueue[idx].type == 1 || eventQueue[idx].type == 2) {
        dropOffset = (int8_t)i;
        break;
      }
    }
    if (dropOffset >= 0) {
      for (uint8_t i = (uint8_t)dropOffset; i < qCount - 1; ++i) {
        uint8_t dst = (qHead + i) % QSIZE;
        uint8_t src = (qHead + i + 1) % QSIZE;
        eventQueue[dst] = eventQueue[src];
      }
      qTail = (qTail + QSIZE - 1) % QSIZE;
      --qCount;
      Serial.println("EVENT_QUEUE_FULL: dropped oldest measurement event");
    } else {
      Serial.println("EVENT_QUEUE_FULL: critical event could not be queued");
      return false;
    }
  }

  eventQueue[qTail] = {type, id, oldValue, newValue, millis()};
  qTail = (qTail + 1) % QSIZE;
  ++qCount;
  return true;
}

bool dequeueEvent(TelemetryEvent &event) {
  if (qCount == 0) return false;
  event = eventQueue[qHead];
  qHead = (qHead + 1) % QSIZE;
  --qCount;
  return true;
}

/* ================= TASK 1: ADC / BATTERY ENGINE ================= */

uint16_t readCellVoltage(uint8_t i) {
  uint16_t raw = analogRead(cellPins[i]);
  rawNow[i] = raw;
  uint32_t mv = (uint32_t)raw * CELL_MAX_MV / ADC_MAX;
  if (mv > CELL_MAX_MV) mv = CELL_MAX_MV;
  return (uint16_t)mv;
}

void readCells() {
  for (uint8_t i = 0; i < NUM_CELLS; ++i) {
    uint16_t voltage = readCellVoltage(i);
    previousV[i] = filteredV[i];

    // Low-pass filter: reduces ADC noise while keeping response reasonably fast.
    if (filteredV[i] == 0) filteredV[i] = voltage;
    else filteredV[i] = ((uint32_t)filteredV[i] * 3 + voltage) / 4;
    cellV[i] = filteredV[i];

    // Frozen sensor heuristic: repeated identical ADC values near either rail.
    if (rawNow[i] == 0 || rawNow[i] >= 4094) {
      if (sameRawCount[i] < 255) ++sameRawCount[i];
    } else {
      sameRawCount[i] = 0;
    }
    lastRaw[i] = rawNow[i];
  }
}

uint16_t calculateAdaptiveThreshold(uint8_t soc) {
  uint16_t threshold;
  if (soc < 20) threshold = 60;
  else if (soc < 40) threshold = 80;
  else if (soc <= 80) threshold = 100;
  else threshold = 120;

  if (threshold < MIN_IMBALANCE_LIMIT_MV) threshold = MIN_IMBALANCE_LIMIT_MV;
  if (threshold > MAX_IMBALANCE_LIMIT_MV) threshold = MAX_IMBALANCE_LIMIT_MV;
  return threshold;
}

void analyzeBattery() {
  weakCell = strongCell = 0;
  for (uint8_t i = 1; i < NUM_CELLS; ++i) {
    if (cellV[i] < cellV[weakCell]) weakCell = i;
    if (cellV[i] > cellV[strongCell]) strongCell = i;
  }

  uint16_t minV = cellV[weakCell];
  uint16_t maxV = cellV[strongCell];
  previousImbalance = imbalance;
  imbalance = maxV - minV;

  if (imbalance > previousImbalance + 10) imbalanceTrend = TREND_INCREASING;
  else if (previousImbalance > imbalance + 10) imbalanceTrend = TREND_DECREASING;
  else imbalanceTrend = TREND_STABLE;

  uint32_t total = 0;
  for (uint8_t i = 0; i < NUM_CELLS; ++i) total += cellV[i];
  uint16_t average = total / NUM_CELLS;

  // Demonstration estimate only; real SoC requires chemistry/current calibration.
  if (average <= 3000) estimatedSOC = 0;
  else if (average >= 4200) estimatedSOC = 100;
  else estimatedSOC = (uint32_t)(average - 3000) * 100 / 1200;

  adaptiveImbalanceLimit = calculateAdaptiveThreshold(estimatedSOC);
}

/* ================= TASKS 2 / 4: FAULT DETECTION ================= */

void detectFaults() {
  adcFault = false;
  cellFault = false;
  frozenADC = false;
  voltageJump = false;
  activeFault = FAULT_NONE;

  for (uint8_t i = 0; i < NUM_CELLS; ++i) {
    if (rawNow[i] == 0 || rawNow[i] >= 4095 || cellV[i] < 100 || cellV[i] > 4300) {
      adcFault = true;
      if (activeFault == FAULT_NONE) activeFault = FAULT_ADC_RANGE;
    }

    int delta = abs((int)cellV[i] - (int)previousV[i]);
    // A large filtered jump is a warning. Threshold avoids ordinary small ADC noise.
    if (previousV[i] != 0 && delta > 500) {
      voltageJump = true;
      if (activeFault == FAULT_NONE) activeFault = FAULT_VOLTAGE_JUMP;
    }

    if (sameRawCount[i] >= 25) {
      frozenADC = true;
      if (activeFault == FAULT_NONE) activeFault = FAULT_ADC_FROZEN;
    }

    if (cellV[i] < UV_LIMIT_MV || cellV[i] > OV_LIMIT_MV) {
      cellFault = true;
      if (activeFault == FAULT_NONE) activeFault = FAULT_CELL_VOLTAGE;
    }
  }

  if (imbalance > adaptiveImbalanceLimit && activeFault == FAULT_NONE) {
    activeFault = FAULT_IMBALANCE;
  }
  if (manualFault) activeFault = FAULT_MANUAL_SHUTDOWN;
  if (commFault && activeFault == FAULT_NONE) activeFault = FAULT_COMMUNICATION;
}

void logStateTransition(SystemState from, SystemState to, FaultId fault) {
  Serial.printf("[t=%lu ms] STATE %s -> %s | FAULT_ID=%u (%s)\n",
                (unsigned long)millis(), stateName(from), stateName(to),
                (unsigned)fault, faultName(fault));
  enqueueEvent(3, (uint8_t)fault, (int32_t)from, (int32_t)to);
  if (to == FAILSAFE || to == SHUTDOWN || fault != FAULT_NONE) ++totalFaults;
}

void updateState() {
  previousState = state;

  bool criticalFault = adcFault || frozenADC || voltageJump;
  bool cellCritical = cellFault;
  bool degradedFault = commFault || (imbalance > adaptiveImbalanceLimit);

  if (manualFault) {
    state = SHUTDOWN;
    activeFault = FAULT_MANUAL_SHUTDOWN;
  } else if (criticalFault || cellCritical) {
    state = FAILSAFE;
    if (activeFault == FAULT_NONE) activeFault = cellCritical ? FAULT_CELL_VOLTAGE : FAULT_ADC_RANGE;
    recoveryPending = false;
  } else if (state == FAILSAFE && !degradedFault) {
    // Remain in FAILSAFE until faults are continuously clear for a verification interval.
    if (!recoveryPending) {
      recoveryPending = true;
      faultClearSince = millis();
    }
    if (millis() - faultClearSince >= RECOVERY_VERIFY_MS) {
      state = NORMAL;
      recoveryPending = false;
      activeFault = FAULT_NONE;
    } else {
      state = FAILSAFE;
    }
  } else if (degradedFault) {
    state = DEGRADED;
    recoveryPending = false;
    if (commFault) activeFault = FAULT_COMMUNICATION;
    else if (activeFault == FAULT_NONE) activeFault = FAULT_IMBALANCE;
  } else {
    state = NORMAL;
    recoveryPending = false;
    activeFault = FAULT_NONE;
  }

  if (state != previousState) logStateTransition(previousState, state, activeFault);

  bool faultNow = (state == DEGRADED || state == FAILSAFE || state == SHUTDOWN);
  if (faultNow != faultWasActive) {
    enqueueEvent(4, (uint8_t)activeFault, faultWasActive ? 1 : 0, faultNow ? 1 : 0);
    faultWasActive = faultNow;
  }
}

/* ================= TASK 2: NON-BLOCKING RELAY ================= */

void updateRelay() {
  bool desired = (state == NORMAL || state == DEGRADED);

  if (desired != relayCommand) {
    relayCommand = desired;
    lastRelayCommandChange = millis();
  }

  // Safety OFF is immediate; turning ON requires a stable command interval.
  if (state == FAILSAFE || state == SHUTDOWN) relayState = false;
  else if (millis() - lastRelayCommandChange >= RELAY_ON_CONFIRM_MS) relayState = relayCommand;

  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
  digitalWrite(LED_PIN, relayState ? HIGH : LOW);

  if (relayState != previousRelayState) {
    enqueueEvent(5, 0, previousRelayState ? 1 : 0, relayState ? 1 : 0);
    previousRelayState = relayState;
  }

  // No relay feedback pin is wired in this Wokwi circuit; physical contact mismatch
  // cannot be verified until a feedback input is added.
}

/* ================= MANUAL BUTTON: DEBOUNCED ================= */

void checkButton() {
  bool reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonReading) {
    lastButtonChange = millis();
    lastButtonReading = reading;
  }

  if (millis() - lastButtonChange >= BUTTON_DEBOUNCE_MS && reading != stableButtonReading) {
    stableButtonReading = reading;
    if (stableButtonReading == LOW) {
      manualFault = !manualFault;
      Serial.printf("[t=%lu ms] MANUAL_FAULT=%s\n",
                    (unsigned long)millis(), manualFault ? "ON" : "OFF");
    }
  }
}

/* ================= TASK 5: WIFI / BLYNK CONNECTION ================= */

void updateWiFi() {
  if (millis() - lastWiFi < WIFI_CHECK_MS) return;
  lastWiFi = millis();

  wifiOnline = (WiFi.status() == WL_CONNECTED);
  if (!wifiOnline) {
    telemetryOnline = false;
    wifiRSSI = -100;
    WiFi.begin(WIFI_SSID, WIFI_PASS); // asynchronous reconnect request
    Serial.println("COMM: Wi-Fi offline; reconnect requested");
  } else {
    wifiRSSI = WiFi.RSSI();
    if (!Blynk.connected()) {
      telemetryOnline = false;
      Blynk.connect(100); // bounded attempt; not a long blocking wait
    }
    telemetryOnline = Blynk.connected();
    Serial.printf("COMM: WiFi=ONLINE Blynk=%s RSSI=%d dBm\n",
                  telemetryOnline ? "ONLINE" : "OFFLINE", wifiRSSI);
  }

  // Wi-Fi alone is not enough: the Blynk session must also be connected.
  commFault = (!wifiOnline || !telemetryOnline);
}

/* ================= TASK 5: BLYNK DASHBOARD ================= */

void sendBlynkData() {
  if (!telemetryOnline || !Blynk.connected()) return;

  for (uint8_t i = 0; i < NUM_CELLS; ++i) Blynk.virtualWrite(i, cellV[i] / 1000.0f);
  Blynk.virtualWrite(V4, weakCell + 1);
  Blynk.virtualWrite(V5, strongCell + 1);
  Blynk.virtualWrite(V6, imbalance);
  Blynk.virtualWrite(V7, stateName(state));
  Blynk.virtualWrite(V8, relayState ? 1 : 0);
  Blynk.virtualWrite(V9, (state == DEGRADED || state == FAILSAFE || state == SHUTDOWN) ? 1 : 0);
  Blynk.virtualWrite(V10, wifiRSSI);
  Blynk.virtualWrite(V11, qCount);
  Blynk.virtualWrite(V12, riskScore);
  Blynk.virtualWrite(V13, estimatedSOC);
  Blynk.virtualWrite(V14, telemetryOnline ? "LIVE" : "OFFLINE");
}

/* ================= TASK 6: RISK / MAINTENANCE ================= */

void calculateRisk() {
  float risk = 0;

  if (imbalance >= adaptiveImbalanceLimit) risk += 30;
  else if (imbalance >= adaptiveImbalanceLimit / 2) risk += 15;

  if (imbalanceTrend == TREND_INCREASING) risk += 15;
  if (estimatedSOC < 20) risk += 25;
  else if (estimatedSOC < 40) risk += 10;

  risk += (totalFaults >= 10) ? 20 : totalFaults * 2;
  if (state == DEGRADED) risk += 15;
  else if (state == FAILSAFE) risk += 35;
  else if (state == SHUTDOWN) risk += 50;

  if (risk > 100) risk = 100;
  riskScore = risk;
}

const char* maintenanceAdvice() {
  if (state == SHUTDOWN) return "Manual shutdown";
  if (state == FAILSAFE) return "Inspect fault now";
  if (imbalance > adaptiveImbalanceLimit) return "Check cell balance";
  if (imbalanceTrend == TREND_INCREASING) return "Watch imbalance";
  if (estimatedSOC < 20) return "Low SOC: recharge";
  if (state == DEGRADED) return "Check connection";
  return "Battery operating OK";
}

/* ================= TASK 5: SIGNIFICANT EVENTS ================= */

void detectEvents() {
  static uint16_t lastCell[NUM_CELLS] = {0};
  static uint16_t lastImbalance = 0;
  static SystemState lastState = NORMAL;
  static bool lastRelay = false;
  static bool baselineReady = false;

  if (!baselineReady) {
    for (uint8_t i = 0; i < NUM_CELLS; ++i) lastCell[i] = cellV[i];
    lastImbalance = imbalance;
    lastState = state;
    lastRelay = relayState;
    baselineReady = true;
    return;
  }

  for (uint8_t i = 0; i < NUM_CELLS; ++i) {
    if (abs((int)cellV[i] - (int)lastCell[i]) >= CELL_EVENT_DELTA_MV) {
      enqueueEvent(1, i + 1, lastCell[i], cellV[i]);
      lastCell[i] = cellV[i];
    }
  }

  if (abs((int)imbalance - (int)lastImbalance) >= IMBALANCE_EVENT_DELTA_MV) {
    enqueueEvent(2, 0, lastImbalance, imbalance);
    lastImbalance = imbalance;
  }

  if (state != lastState) {
    // State transition is already logged by updateState; avoid duplicating it here.
    lastState = state;
  }

  if (relayState != lastRelay) {
    enqueueEvent(5, 0, lastRelay ? 1 : 0, relayState ? 1 : 0);
    lastRelay = relayState;
  }
}

void replayQueue() {
  if (!wifiOnline || !telemetryOnline || !Blynk.connected() || qCount == 0) return;
  if (millis() - lastReplay < 500) return;
  lastReplay = millis();

  // Keep the FIFO head until the event's datastream writes have been issued.
  // Blynk virtualWrite is best-effort; it does not provide a server ACK.
  TelemetryEvent e = eventQueue[qHead];

  Blynk.virtualWrite(V15, (int)e.type);
  Blynk.virtualWrite(V16, (int)e.id);
  Blynk.virtualWrite(V17, (int)e.oldValue);
  Blynk.virtualWrite(V18, (int)e.newValue);
  Blynk.virtualWrite(V19, (int)(e.timestamp / 1000UL));

  Serial.printf("[t=%lu ms] QUEUE_REPLAY type=%u id=%u %ld->%ld remaining=%u\\n",
                (unsigned long)e.timestamp, e.type, e.id,
                (long)e.oldValue, (long)e.newValue, qCount - 1);

  // Best-effort removal after issuing all publication calls.
  qHead = (qHead + 1) % QSIZE;
  --qCount;
}

/* ================= TASK 3: FLICKER-FREE LCD ================= */

void lcdWriteLine(uint8_t row, const char* text) {
  char buffer[17];
  snprintf(buffer, sizeof(buffer), "%-16.16s", text);

  if (strncmp(buffer, lcdLast[row], 16) == 0) return;
  lcd.setCursor(0, row);
  lcd.print(buffer);
  memcpy(lcdLast[row], buffer, 17);
}

void updateLCD() {
  if (millis() - lastLCD < LCD_REFRESH_MS) return;
  lastLCD = millis();

  char line1[17] = {0};
  char line2[17] = {0};

  if (state == FAILSAFE || state == SHUTDOWN) {
    snprintf(line1, sizeof(line1), "%s", stateName(state));
    snprintf(line2, sizeof(line2), "FAULT:%s", faultName(activeFault));
  } else if (lcdPage == 0) {
    snprintf(line1, sizeof(line1), "C1 %.2f C2 %.2f", cellV[0] / 1000.0f, cellV[1] / 1000.0f);
    snprintf(line2, sizeof(line2), "C3 %.2f C4 %.2f", cellV[2] / 1000.0f, cellV[3] / 1000.0f);
  } else if (lcdPage == 1) {
    snprintf(line1, sizeof(line1), "STATE:%s", stateName(state));
    snprintf(line2, sizeof(line2), "W%u S%u I%umV", weakCell + 1, strongCell + 1, imbalance);
  } else if (lcdPage == 2) {
    snprintf(line1, sizeof(line1), "WiFi:%s", wifiOnline ? "ON" : "OFF");
    snprintf(line2, sizeof(line2), "Q%u/%u R%u", qCount, QSIZE, (unsigned)riskScore);
  } else {
    snprintf(line1, sizeof(line1), "SOC:%u%% R:%u%%", estimatedSOC, (unsigned)riskScore);
    snprintf(line2, sizeof(line2), "Relay:%s", relayState ? "ON" : "OFF");
  }

  lcdWriteLine(0, line1);
  lcdWriteLine(1, line2);
}

void rotateLCDPage() {
  if (millis() - lastPage >= LCD_PAGE_MS) {
    lastPage = millis();
    lcdPage = (lcdPage + 1) % 4;
  }
}

/* ================= SERIAL TELEMETRY / REPORT ================= */

void sendSerialTelemetry() {
  Serial.println("\n========== ESP32 INTELLIGENT BMS ==========");
  for (uint8_t i = 0; i < NUM_CELLS; ++i) {
    Serial.printf("CELL%u=%.3fV\n", i + 1, cellV[i] / 1000.0f);
  }

  Serial.printf("WEAK=C%u STRONG=C%u\n", weakCell + 1, strongCell + 1);
  Serial.printf("IMBALANCE=%umV LIMIT=%umV TREND=%s\n",
                imbalance, adaptiveImbalanceLimit, trendName(imbalanceTrend));
  Serial.printf("SOC=%u%% STATE=%s FAULT_ID=%u (%s)\n",
                estimatedSOC, stateName(state), (unsigned)activeFault, faultName(activeFault));
  Serial.printf("RELAY=%s WIFI=%s RSSI=%d QUEUE=%u/%u\n",
                relayState ? "ON" : "OFF", wifiOnline ? "ONLINE" : "OFFLINE",
                wifiRSSI, qCount, QSIZE);
  Serial.printf("RISK=%.1f%% UPTIME=%lu s FAULT_COUNT=%lu\n",
                riskScore, (unsigned long)((millis() - uptimeStart) / 1000),
                (unsigned long)totalFaults);
  Serial.printf("ADVICE=%s\n", maintenanceAdvice());
  Serial.println("===========================================\n");
}

/* ================= SETUP ================= */

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  analogReadResolution(12);
  for (uint8_t i = 0; i < NUM_CELLS; ++i) {
    pinMode(cellPins[i], INPUT);
    analogSetPinAttenuation(cellPins[i], ADC_11db);
  }

  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcdWriteLine(0, "ESP32 BMS");
  lcdWriteLine(1, "Starting...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Blynk.config(BLYNK_AUTH_TOKEN);

  uptimeStart = millis();
  Serial.println("ESP32 Intelligent BMS starting");
  Serial.println("Tasks 1-6 enabled; loop uses millis()-based scheduling");
}

/* ================= MAIN LOOP: NO delay() ================= */

void loop() {
  Blynk.run();
  uint32_t now = millis();

  if (now - lastSensor >= SENSOR_INTERVAL_MS) {
    lastSensor = now;
    readCells();
    analyzeBattery();
    detectFaults();
    updateState();
    updateRelay();
    detectEvents();
  }

  checkButton();
  updateWiFi();

  if (now - lastAnalytics >= ANALYTICS_INTERVAL_MS) {
    lastAnalytics = now;
    calculateRisk();
  }

  rotateLCDPage();
  updateLCD();

  if (now - lastTelemetry >= TELEMETRY_INTERVAL_MS) {
    lastTelemetry = now;
    sendSerialTelemetry();
    sendBlynkData();
  }

  replayQueue();
}
