# ESP32-Based Intelligent Battery Management System

## Project Overview

This project implements an ESP32-based Battery Management System (BMS) prototype for monitoring battery cell voltages, detecting abnormal operating conditions, and presenting battery status through an LCD and a Blynk IoT dashboard.

The project focuses on modular battery analysis, non-blocking protection logic, fault-state management, event-driven telemetry, and battery health analytics.

## Objectives

- Monitor individual cell voltages.
- Identify the weakest and strongest cells.
- Calculate voltage imbalance and observe its trend.
- Implement relay protection and fault handling.
- Display battery information on an LCD.
- Monitor system status through Blynk.
- Explore offline telemetry handling and risk analysis.

## Main Features

1. Modular battery monitoring engine.
2. Non-blocking relay protection.
3. LCD information-page rotation.
4. Four-state fault management:
   - NORMAL
   - DEGRADED
   - FAILSAFE
   - SHUTDOWN
5. Wi-Fi and Blynk telemetry.
6. Battery risk analysis and maintenance recommendations.

## Hardware Components

- ESP32 development board
- Four potentiometers for simulated cell-voltage inputs
- I2C LCD
- Relay module or simulated relay output
- Push button
- LED and current-limiting resistor
- Jumper wires

## Software and Tools

- Arduino IDE
- ESP32 Arduino Core
- Wokwi Simulator
- Blynk IoT Platform
- GitHub

## System Architecture

Cell-voltage inputs are read by the ESP32. The monitoring engine processes the readings and calculates battery status. Protection logic evaluates faults and controls the relay output. The LCD presents local status, while Blynk provides remote monitoring and telemetry.

## Testing and Verification

The prototype is intended to be evaluated using:

- Cell-voltage variation
- Weakest and strongest cell identification
- Voltage-imbalance monitoring
- Fault injection and recovery
- LCD page rotation
- Wi-Fi disconnection and reconnection
- Blynk dashboard updates

Actual test results and supporting screenshots will be documented separately.

## Limitations

This is a prototype. Potentiometers used as cell inputs simulate voltage measurements and do not replace a dedicated battery-monitoring IC.

Hardware protection, sensor-failure detection, relay feedback, persistent fault history, and reliable cloud event replay must be verified against the final implementation before being claimed as fully implemented.

## Safety Notice

This prototype must not be connected directly to a high-voltage or high-energy battery pack without appropriate sensing, isolation, protection circuitry, and hardware validation.

## Author

Parth Khairnar

Electronics and Telecommunication Engineering
