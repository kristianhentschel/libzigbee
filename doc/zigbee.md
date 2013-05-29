[Table of Contents](toc.md)


ZigBee radio setup and configuration
====================================

The ZigBee radio transceivers (we got ours as XBee S2 branded modules from Sparkfun [sparkfun-xbee]) come in some random configuration. For starters, they need to be programmed with a different firmware version depending on whether you want them to run as a `coordinator`, `end device`, or `router`. _For our small star topology network, we programmed one as coordinator and all others as routers for simplicity._

For wider reading on the differences between these modes, I would recommend at least skimming the [zigbee-datasheet].

XBee Firmware Programming
---------------------------
To my knowledge, the simplest way to change a ZigBee module's firmware and experiment with the options is through Digi's X-CTU software [xctu]. This requires access to a Windows computer and some way to connect the ZigBee module to your computer's serial port.

### ZigBee to PC connection
Digi recommends purchasing a $25 adapter such as the XBee Explorer USB: [sparkfun-explorer]

Instead, we used an Arduino Uno (with the microcontroller removed) we had already lying around, and built a simple level-converter from 2 resistors and a 74-series AND gate on a breadboard, which worked just as well. 

![Arduino to XBee level converter](img/arduino-xbee-levels.png)

### Firmware Programming
Once you have connected your XBee module to your computer and installed the X-CTU software, you should confirm that the module is detected by the software. Then switch to the `Modem Configuration` tab. Press `Read` to display the current configuration. Then select the desired firmware version (You'll want one of the three _ZIGBEE ... API_ versions) from the `Function Set` dropdown menu, and update your module by clicking `Write`.

ZigBee Options
--------------
There are various options to configure. While your application can easily change most of these at runtime through an AT command, it may be simpler to set them from a PC. The most important options are listed here, more information may be found in the datasheet linked at the top of this page.

* **PAN ID** should be set to the same value on all devices. This makes sure all non-coordinator devices only join the network set up by your coordinator.
* **Node Identifier** is a string name for the current module, we found this helpful for debugging our network by printing this at application startup.
* **Baud Rate** is the rate used for communicating with the host device over the serial port (RX, TX). Do not forget what you set this to, as it also needs to be set in X-CTU or your module won't be recognised anymore. We found leaving it at the default 9600 baud to work fine.
* **Sleep Modes** Read the datasheet before touching these. For debugging and application development where power is not a serious concern, I'd recommending sending all routers to NO SLEEP mode, as it can be difficult waking them up later.



[zigbee-datasheet]: ftp://ftp1.digi.com/support/documentation/90000866_A.pdf
[sparkfun-xbee]: https://www.sparkfun.com/products/10414 
[sparkfun-explorer]: https://www.sparkfun.com/products/8687 
[xctu]: http://www.digi.com/support/productdetail?pid=3352
