# Assumptions Document

**Project:** ESP32 Intelligent Battery Management System (BMS)  
**Internship:** ElevanceSkills — Embedded Systems

## 1. Simulation Environment
The current project is developed and demonstrated using Wokwi simulation and Blynk IoT dashboard. The results do not represent testing on a physical EV battery pack.

## 2. Cell Voltage Measurement
Four potentiometers represent the four cell-voltage inputs in the simulation. They are used for testing firmware behavior and are not isolated battery measurement circuits.

## 3. Cell Count
The current demonstrated configuration supports four simulated cell inputs. Support for 8, 12, or 16 cells requires additional code refactoring and verification.

## 4. Relay Operation
The relay status represents the firmware-controlled output. Independent physical relay-contact feedback is not connected in the current setup.

## 5. Fault Detection
Only fault scenarios demonstrated through repeatable tests should be considered verified. Frozen ADC readings within the normal range and other untested fault scenarios may not be detected reliably.

## 6. Telemetry and Connectivity
The project uses Wi-Fi and Blynk for telemetry. Offline queue data is stored in memory and may be lost after a restart. Exact ordered replay of original event payloads has not been fully verified.

## 7. Voltage Thresholds
Voltage thresholds and recovery conditions are firmware-configured simulation values. They must be validated for the selected cell chemistry and hardware before any real-world deployment.

## 8. Safety Limitation
This is an educational prototype. It is not a certified battery protection system and must not be used as the sole safety mechanism for a real EV battery.

## 9. Verification Policy
Project documentation must distinguish between demonstrated functionality, known limitations, and features requiring further testing.

## 10. Future Improvements
- Add reliable frozen-sensor detection.
- Add independent relay feedback.
- Improve ordered telemetry replay.
- Refactor the code for scalable cell configurations.
- Add persistent fault history.
- Perform systematic testing on all supported fault conditions.
