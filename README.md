PSGroove
========

This is the PSGroove, an open-source reimplementation of the psjailbreak exploit for
AT90USB and related microcontrollers.

It is known to work on:

- AT90USB162
- AT90USB646
- AT90USB647
- AT90USB1286
- AT90USB1287
- ATMEGA32U2
- ATMEGA32U4

... and maybe more.

**This software is not intended to enable piracy, and such features
have been disabled.  This software is intended to allow the execution
of unsigned third-party apps and games on the PS3.**

No one involved in maintaining the psgroove git is responsible for or has any involvement with any existing usb dongles sporting "psgroove" in its name. Thank you for your understanding.

Cloning
-------
The repository uses the LUFA library as a submodule.  To clone, use something like:

    git clone git://github.com/psgroove/psgroove.git
    cd psgroove
    git submodule init
    git submodule update

If you don't have PPU-GCC installed, make might get confused and refuse to build. To fix this do something like:

    cd PL3
    make clean
    git checkout .
    cd ..

Make should now work as expected and use the precompiled PL3 payloads.

Configuring
-----------

This version of PSGroove has been modified to directly use PL3 payloads instead of a single hardcoded Payload for much greater flexablity.

Edit Makefile to reflect your firmware version (3_41, 3_01, 3_10 and 3_15 are currently supported) and board.

Alternately, you can just use the build_hex.sh to automatically build hex files for all supported boards and firmware versions.

By default PSGroove is configured to use the dev PL3 payload which matches the peek/poke payload that PSGroove used to have. You can select another PL3 payload by changing the PAYLOAD define in descriptor.h 

Board-specific notes
--------------------
Teensy boards only have one LED, so it will turn off when the exploit
succeeds rather than turn green.  Older Teensy 1.0 boards also have
the polarity inverted.  In general, a LED should do something when the
board is powered, and do something different when the exploit works.


Building
--------
On Linux, use the AVR GCC toolchain (Debian/Ubuntu package: gcc-avr).
On Windows, WinAVR should do the trick.

    make clean
    make


Programming
-----------
Now program psgroove.hex into your board and you're ready to go.  For
the AT90USBKEY and other chips with a DFU bootloader preinstalled, you
can get the dfu-programmer tool, put your board in programming mode,
and run
  
    make dfu

For the Teensy boards, you probably have to use the [Teensy
Loader](http://www.pjrc.com/teensy/loader.html) software.

Using
-----
To use this exploit:
  
* Hard power cycle your PS3 (using the switch in back, or unplug it)
* Plug the dongle into your PS3.
* Press the PS3 power button, followed quickly by the eject button.

After a few seconds, the first LED on your dongle should light up.
After about 5 seconds, the second LED will light up (or the LED will
just go off, if you only have one).  This means the exploit worked!
You can see the new "Install Package Files" menu option in the game
menu.


Notes
-----
A programmed dongle won't enumerate properly on a PC, so don't worry
about that.

