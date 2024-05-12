# CPE 301 - Final Project: Evaporative Cooling System

## University of Nevada, Reno

### Embedded System Design - Spring 2024

---

## Overview

This project aims to create an evaporation cooling system (a swamp cooler), which provides an energy-efficient alternative to air conditioners in dry, hot climates. The system uses an Arduino 2560 and various sensors to monitor and control the cooling process.

## Project Requirements

- Monitoring water levels in the reservoir with alerts for low levels.
- Displaying air temperature and humidity on an LCD screen.
- Controlling fan motor based on temperature range.
- On/off system control via a user button.
- Logging time and date of motor activation/deactivation.

## Hardware Component Selection

- Water level sensor for monitoring with threshold detection.
- LCD display for output messages.
- Real-time clock module for event reporting.
- DHT11 sensor for temperature and humidity readings.
- Motor and fan blade from the kit with a separate power supply board.

![State Diagram](https://github.com/MattStanl3y/301FinalProject/blob/main/diagram.jpeg)

## System States

The cooler operates in several states: DISABLED, IDLE, ERROR, and RUNNING. Each state has specific requirements for operation and LED indication.

## Deliverables

1. Project Overview Document - A PDF with design overview, constraints, and pictures.
2. GitHub Repository - A repository with all code, schematics, and team member contributions.
3. Operation Video - A demonstration video with narration.

## Rubrics

Detailed rubrics include points deduction for missing videos, technical documents, GitHub submissions, and functionality.

---

### Note to Team Members

Please ensure all commits are properly commented and that the repository is set to public. Adhere to all project requirements and use lab concepts effectively.

---

## Group Information

- Team Members:
  - Matt Stanley
  - Jason Parmar

## Links

- [GitHub Repository](https://github.com/MattStanl3y/301FinalProject)
- [Demonstration Video](https://drive.google.com/file/d/1-cBQ-8gxt2DP7--STzRgNJXBkg23tMsw/view?usp=sharing)
- [Technical Document](...)
