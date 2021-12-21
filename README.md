# SATCOM Modem Interface

SATCOM Modem Interface MCU for the SATCOM Relay

This repo contains the source code for the SATCOM Modem Interface components of the SATCOM Relay project. See the [Relay repo](https://github.com/IQTLabs/satcom-relay) for:
- BOM
- Wiring Diagram
- RockBlock to Feather Adapter PCB

## Arduino Environment Setup

### MCU
- Adafruit [Feather M0 Adalogger](https://www.adafruit.com/product/2796) 

### Arduino Libraries for this Repo

- IridiumSBDi2c by SparkFun Electronics [(datasheet)](https://docs.rockblock.rock7.com/docs/connectors)
- ArduinoJson by Benoit Blanchon

## Operations

### High-level "loop" Operations

- sleep
- wake on interrupt
- receive message on SERCOM1 pins
- save to SD card queue
- send messages in queue over Iridium