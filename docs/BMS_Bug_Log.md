# BMS Bug Log

**Project:** ESP32 Intelligent Battery Management System  
**Internship:** ElevanceSkills — Embedded Systems

## Purpose
This document records observed issues, verification results, known limitations, and planned improvements.

## B-01: Cell Voltage Imbalance
- Observation: Unequal simulated cell voltages can cause the system to enter DEGRADED state.
- Evidence: A DEGRADED-state simulation was demonstrated.
- Follow-up: Test multiple voltage combinations and document the configured threshold.

## B-02: Cell Voltage Fault
- Observation: A simulated cell voltage of approximately 2.955 V was used during fault testing.
- Result: FAILSAFE state, CELL_VOLTAGE fault ID, and relay OFF were observed.
- Limitation: This is simulation evidence, not validation on a real battery pack.

## B-03: Fault Recovery
- Observation: Normal simulated cell values were restored after a fault.
- Result: NORMAL state, fault ID NONE, and relay ON were observed in the demonstrated scenario.
- Follow-up: Test recovery for every supported fault type.

## B-04: Frozen ADC Reading
- Limitation: The current implementation primarily detects repeated ADC rail values. A frozen reading within the normal ADC range may not be detected.
- Improvement: Add per-channel frozen-reading detection and test it.

## B-05: Relay Feedback
- Limitation: Independent physical relay-contact feedback is not wired in the current simulation.
- Improvement: Add suitable feedback hardware before claiming physical relay-mismatch detection.

## B-06: Offline Telemetry Replay
- Limitation: Queue-depth behavior was demonstrated, but replay does not currently preserve every original event payload.
- Improvement: Store event timestamps and payloads, then verify ordered FIFO replay.

## B-07: Cell-Count Scaling
- Limitation: The current pin configuration and display logic are designed for four cell inputs. Changing only NUM_CELLS does not establish complete 8–16 cell support.
- Improvement: Refactor cell acquisition and display logic, then test larger configurations.

## B-08: Persistent Fault History
- Limitation: Fault-history persistence across reboot has not been demonstrated.
- Improvement: Implement and test persistent storage.

## Status
Only the scenarios supported by repeatable evidence should be marked as verified. Known limitations must remain documented.
