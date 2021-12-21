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

### SD Card Queueing

The `MessageLog` class implements a LIFO queue/stack with `push(String)` and
`pop()` methods for storing and retrieving messages. Each `MessageLog` instance
operates on a single underlying file, the name of which is passed in the
constructor.

As messages are received they are `push()`ed onto the `unsentMessageLog` queue.
When ready to send messages over Iridium, they are `pop()`ped from the
`unsentMessageLog` queue, sent, and `push()`ed onto the `sendMessageLog` queue.
