libzigbee (working title)
=========================

This library is intended to provide a common interface on both linux machines (in particular, the RaspberryPi) and small embedded devices (e.g. the ST Discovery ARM Cortex M series) to the Zigbee2 family of radio transceivers. This guide will explain the library API after covering setup of the transceivers, operating systems, and embedded devices.

**DISCLAIMER:** This documentation is currently under construction and most sections are written based on memory and old project notes. I describe what worked best for me, and that may not necessarily be the best way to do something. If you have anything to add, feel free to email me, comment, or fork the repository and submit a pull request.


Prerequisites
-------------

These documents outline the required setup and connections to be made for getting the ZigBee radios to work in first place, as well as for connecting them to a Linux PC/Raspberry Pi or an embedded microcontroller board.

1. [ZigBEE radio setup and Configuration](zigbee.md)
2. [Raspberry Pi (Linux) setup and ZigBee connection](raspberrypi.md)
3. [Embedded Device Setup and ZigBee connection](embedded.md)

API Documentation
-----------------

This section describes the usage of the libzigbee library.

1. [Getting Started Guide](gettingstarted.md)
2. [Transpot layer API](transport.md)
3. [Packet layer API](packets.md)
