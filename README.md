PSGroove
========

This is the PSGroove, an open-source reimplementation of the psjailbreak exploit for
AT90USB and related microcontrollers.

It should work on:

- AT90USB162
- AT90USB646
- AT90USB647
- AT90USB1286
- AT90USB1287
- ATMEGA32U4

... and maybe more.

**This software is not intended to enable piracy, and such features
have been disabled.  This software is intended to allow the execution
of unsigned third-party apps and games on the PS3.**

Cloning
-------
The repository uses the LUFA library as a submodule.  To clone, use something like:

    git clone git://github.com/psgroove/psgroove.git
    cd psgroove
    git submodule init
    git submodule update


Configuring
-----------
Chip and board selection can usually be handled in the Makefile.
In particular, update the MCU, BOARD, and F_CPU lines.  Suggested values:

Teensy 1.0:
 
    MCU = at90usb162
    BOARD = TEENSY
    F_CLOCK = 16000000

Teensy++ 1.0:
 
    MCU = at90usb646
    BOARD = TEENSY
    F_CLOCK = 16000000

Teensy 2.0:

    MCU = atmega32u4
    BOARD = TEENSY
    F_CLOCK = 16000000

Teensy++ 2.0:
 
    MCU = at90usb1286
    BOARD = TEENSY
    F_CLOCK = 16000000

AT90USBKEY / AT90USBKEY2:

    MCU = at90usb1287
    BOARD = USBKEY
    F_CLOCK = 8000000

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

This branch has a modified payload that adds peek and poke syscalls 
to the lv2 kernel. A userspace application can use these syscalls to 
dump out the entire memory space of the kernel, or patch the kernel
as it is running.  

Unfortunately, because the free toolchain/sdk is not ready, we can't
distribute an application to do the dumping, so you will have to make
your own.

The lv2 kernel starts at 0x8000000000000000

Peek
----
 * Syscall 6.
 * r3 is a 64 bit address to read
 * A 64 bit value will be returned in r3

Poke
----
 * Syscall 7.
 * r4 is a 64 bit value
 * r3 is the address to write that value to
