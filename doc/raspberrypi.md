[Table of Contents](toc.md)

Raspberry Pi Setup and Preparation
==================================

Physical connection to XBee
---------------------------
The Raspberry Pi comes with a built in serial port that is accessible to theoperating system. It uses the GPIO pins 13 (RX) and 14 (TX). So in theory, the XBee's pin 1 (DIN) could just be connected to the Raspberry Pi's pin 14 (TX), and similarly for the RX and DOUT pins. However: 

_Note that any circuit using the 5V supply must use level conversion for any pins driven by an external device, as the Raspberry Pi does not include overvoltage protection_

**TODO:** Add a circuit diagram or link for example circuit.

Linux Configuration
-------------------

We used Raspbian, which is a Debian Linux distribution optimised for the Raspberry Pi. Other distributions will likely work similarly.

The serial port described above can be accessed as a device named `/dev/ttyAMA0/`. However, the Raspbian distribution is configured to write boot debug messages to this device, so this behaviour has to be disabled in the `/boot/cmdline.txt` and `/etc/inittab/` files. For more details of what exactly neeeds to be done, see [1].

**TODO:** include summary of article in case it becomes unavailable.

[1]: http://www.irrational.net/2012/04/19/using-the-raspberry-pis-serial-port/
