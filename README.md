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

## Operations

### High-level "loop" Operations

- sleep
- wake on interrupt
  - receive message on `SERCOM1` pins
  - save to SD card
  - send messages over Iridium

### SD Card Queueing

The `MessageLog` class abstracts the saving of messages to an SD card. Some
basic failure recovery as well as optionally operating an activity LED is also
implemented. A `new MessageLog(...)` is defined during `setup()` and subsequent
messages can be appended to the log using the `append(String *)` method.

Each call to `append()` will reestablish SD card functionality in case the card
was removed or a transient error occurred.
