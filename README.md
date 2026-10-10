# ESP32-Based Intelligent Battery Management System (BMS)

An ESP32-based battery monitoring prototype featuring cell-voltage analysis,
voltage-imbalance detection, relay protection logic, fault-state management,
LCD status display, and Blynk IoT monitoring.

Developed as an Electronics and Telecommunication Engineering project to
explore embedded systems, battery monitoring, fault handling, and IoT telemetry.

---

## Project Overview

The Intelligent Battery Management System (BMS) is designed to monitor
multiple simulated battery-cell voltages and identify abnormal operating
conditions.

The ESP32 processes voltage readings, identifies the weakest and strongest
cells, calculates voltage imbalance, and evaluates the system's operating
state. Battery information is presented locally through an LCD and remotely
through a Blynk IoT dashboard.

The prototype also explores non-blocking protection logic, fault recovery,
Wi-Fi connectivity handling, and battery-risk analysis.

**Note:** The current implementation uses potentiometers to simulate
cell-voltage inputs. It is a prototype and is not a production-ready EV BMS.

---

## Project Objectives

- Monitor individual simulated cell voltages.
- Identify the weakest and strongest cells.
- Calculate voltage imbalance between cells.
- Observe whether voltage imbalance is increasing or decreasing.
- Evaluate battery operating conditions.
- Implement relay-control and fault-handling logic.
- Display battery information using an LCD.
- Monitor system parameters through a Blynk IoT dashboard.
- Explore connectivity recovery and telemetry handling.
- Calculate a prototype battery-risk score and provide maintenance guidance.

---

## Key Features

### 1. Multi-Cell Voltage Monitoring

- Reads simulated cell-voltage inputs using ESP32 analog input pins.
- Processes multiple cell readings.
- Identifies the minimum and maximum measured cell voltages.
- Calculates voltage imbalance.

### 2. Voltage-Imbalance Analysis

Voltage imbalance is calculated as:

\[
V_{\text{imbalance}} = V_{\text{maximum}} - V_{\text{minimum}}
\]

The result helps identify differences between the simulated cell voltages.

The prototype also evaluates imbalance trends by comparing measurements
over time.

### 3. Adaptive Imbalance Threshold

The monitoring logic explores an adaptive voltage-imbalance threshold
based on estimated battery state of charge (SoC).

This is a prototype algorithm. It should not be treated as a validated
battery-safety threshold for real battery packs.

### 4. Relay Protection Logic

The system evaluates battery conditions and controls a relay output
according to the implemented protection logic.

The prototype explores non-blocking operation and fault recovery.

Actual contact feedback and independent hardware protection are not
implemented or verified by the simulated relay output alone.

### 5. Four-State Fault Management

The system uses four logical operating states:

| State | Description |
|---|---|
| NORMAL | No active condition requiring a higher-severity state |
| DEGRADED | An abnormal or warning condition is present |
| FAILSAFE | A serious detected condition requires protective action |
| SHUTDOWN | The system is placed in a shutdown state |

State transitions depend on the implemented fault-detection and recovery logic.

### 6. LCD Status Display

The LCD provides local battery and system information.

The prototype supports rotating information pages and gives priority
to critical fault information according to the implemented display logic.

### 7. Blynk IoT Monitoring

The Blynk dashboard is used to monitor selected system parameters remotely.

Dashboard information includes:

- Cell voltages
- Weakest and strongest cell information
- Voltage imbalance
- System state
- Relay status
- Fault status
- Wi-Fi signal information
- Telemetry queue information
- Prototype risk score

The values displayed depend on the configured Blynk datastreams and
the running firmware.

### 8. Battery-Risk Analysis

The prototype calculates a risk score using selected battery-monitoring
indicators and provides maintenance guidance.

This score is an experimental indicator, not a certified battery-health
or State of Health (SoH) measurement.

---

## Hardware Components

| Component | Purpose |
|---|---|
| ESP32 development board | Main controller |
| Four potentiometers | Simulated cell-voltage inputs |
| I2C LCD | Local status display |
| Relay module or simulated relay output | Demonstration of protective switching |
| Push button | Manual control or shutdown demonstration |
| LED | Status indication |
| Current-limiting resistor | LED current limiting |
| Jumper wires | Circuit connections |

---

## Pin Configuration

The following pin assignments are used in the prototype configuration.

| Component | ESP32 Pin |
|---|---|
| Cell 1 potentiometer output | GPIO 34 |
| Cell 2 potentiometer output | GPIO 35 |
| Cell 3 potentiometer output | GPIO 32 |
| Cell 4 potentiometer output | GPIO 33 |
| LCD SDA | GPIO 21 |
| LCD SCL | GPIO 22 |
| Relay control | GPIO 25 |
| LED | GPIO 26 |
| Push button | GPIO 27 |

### Connection Notes

- Connect the LCD SDA and SCL pins to the corresponding ESP32 I2C pins.
- Connect each potentiometer's output to its assigned analog input.
- Connect potentiometer supply and ground appropriately.
- Configure the push button according to the firmware's input logic.
- Use a suitable current-limiting resistor with the LED.
- Ensure that the relay module's electrical requirements are compatible
  with the controller and circuit.

These connections describe the prototype configuration. Verify the actual
wiring before reproducing the circuit.

---

## Software and Tools

| Tool | Purpose |
|---|---|
| Arduino IDE | Firmware development and compilation |
| ESP32 Arduino Core | ESP32 development support |
| Wokwi | Circuit simulation and testing |
| Blynk IoT | Remote monitoring dashboard |
| GitHub | Source-code version control and documentation |

---

## System Architecture

The system follows this general processing flow:

1. **Input Layer:** Potentiometers provide simulated cell-voltage inputs.
2. **Measurement Layer:** The ESP32 reads and processes analog readings.
3. **Monitoring Layer:** The firmware calculates cell-voltage statistics
   and voltage imbalance.
4. **Protection Layer:** Fault conditions are evaluated and relay-control
   decisions are made.
5. **State Management Layer:** The operating state is updated according
   to the implemented state-transition logic.
6. **Display Layer:** The LCD presents local status and fault information.
7. **Telemetry Layer:** Selected parameters are sent to the Blynk dashboard.
8. **Analysis Layer:** The prototype evaluates selected indicators for
   battery-risk analysis.

---

## Project Screenshots

The following screenshots demonstrate selected simulation and monitoring
results from the project.

### Normal Battery Operation

![Normal Battery Operation](normal-operation.png)

### Cell Voltage Imbalance

![Cell Voltage Imbalance](cell-imbalance.png)

### Fault Detection and FAILSAFE

![FAILSAFE Fault Detection](failsafe-detection.png)

### Recovery to Normal State

![Recovery to Normal](recover%20to%20normal.png)

### LCD Display

![LCD Display](lcd-display.png)

### Live Blynk Dashboard

![Live Blynk Dashboard](blynk-live%20dashboard.png)

### Historical Blynk Dashboard

![Historical Blynk Dashboard](blynk-history%20dashboard.png)

### Wi-Fi Disconnection Test

![Wi-Fi Disconnection](WIFI-denoting.png)

---

## Testing and Verification

The prototype can be evaluated through the following test scenarios.

| Test | Procedure | Expected Observation |
|---|---|---|
| Cell-voltage monitoring | Adjust the potentiometers | Displayed readings change |
| Weakest/strongest cell | Set different input voltages | Minimum and maximum cells are identified |
| Voltage imbalance | Increase the difference between cell inputs | Calculated imbalance changes |
| Fault injection | Set an input to an abnormal voltage | Fault logic evaluates the condition |
| Fault recovery | Restore the input to a safe range | Recovery logic evaluates whether normal operation can resume |
| LCD page rotation | Allow the display to run | Information pages rotate |
| Wi-Fi interruption | Disconnect the network | Connectivity status changes |
| Blynk monitoring | Observe the dashboard | Configured values update when telemetry is available |

Actual observations should be recorded with test evidence. Expected behavior
does not, by itself, establish that every test has passed.

---

## Limitations

- Potentiometers simulate cell voltages and do not replace a dedicated
  battery-monitoring IC.
- The prototype is configured for four simulated cell inputs; scaling to
  larger cell counts requires suitable hardware inputs and corresponding
  firmware changes.
- The adaptive imbalance threshold is experimental and requires validation
  for a specific battery chemistry and operating condition.
- A simulated relay output does not prove that a physical relay has switched
  correctly.
- Independent hardware protection and relay-contact feedback require
  additional circuitry.
- Sensor-failure detection must be validated for the relevant failure modes.
- Reliable cloud event replay and persistent fault history require
  implementation and testing beyond basic dashboard updates.
- The prototype risk score is not a certified battery-health measurement.
- Real battery charging, discharging, balancing, and safety performance
  have not been established by potentiometer-based simulation.

---

## Future Improvements

- Integrate a dedicated battery-monitoring IC.
- Add validated current and temperature measurement.
- Implement hardware-backed overvoltage and undervoltage protection.
- Add relay-contact feedback and fault diagnostics.
- Improve persistent fault logging and telemetry recovery.
- Validate the system with controlled hardware tests.
- Expand the architecture to support additional cells.
- Validate risk analysis using measured battery data.
- Design a dedicated PCB after validating the circuit and safety requirements.

---

## Safety Notice

This is an educational prototype, not a certified battery-management system.

Do not connect this prototype directly to a high-voltage or high-energy
battery pack. Real battery applications require appropriate cell-monitoring
hardware, isolation, current and temperature protection, correctly rated
switching devices, and professional hardware validation.

---

## Author

**Parth Khairnar**

Electronics and Telecommunication Engineering  
Dr. D. Y. Patil College of Engineering, Akurdi, Pune

GitHub: [khairnarparth291-crypto](https://github.com/khairnarparth291-crypto)
