The Soundcraft Notepad USB protocol
===================================

This document describes the USB protocol spoken between the
"Soundcraft USB audio control panel" and a Soundcraft Notepad mixer
and the mixer state that protocol communicates about.


General packet format
=====================

The observed USB control request packets all have these properties:

    USB Control transfer with
         0x40  bmRequestType
         16    bRequest
         0     wValue
         0     wIndex

    8 byte command data buffer

There are other USB requests in use, e.g. for getting the meter
value in the vendor GUI's "Ducking" tab.


Mixer state vs host software state
==================================

The MacOS "Soundcraft USB audio control panel" does not appear to
query any information from the mixer when the Mac is up and running in
normal operation. On starting the "Soundcraft USB audio control
panel", no USB requests can be observed which query the mixer
state. However, the "Soundcraft USB audio control panel" remembers the
last state it set the mixer to when it is started the next time, which
means that some software component must keep track of the state the
mixer has been set to.


Audio Routing
=============

The "Audio Routing" tab allows us to select any of the four different
selections for the audio input.

This is the format of the "set audio routing" command:

    00 00 04 00 00 00 00 00
                ┗┩
                 └─────────── source index (range 0..3)

The table of audio routing options for the Notepad-5:

    index     device label     description from the manual
    0         MIC 1 + LINE 2   Mic Input 1 + Mono line input 2
    1         LINE 5+6         Stereo Input 2+3
    2         LINE 7+8         Stereo Input 4+5
    3         MASTER L+R       Mix L+R

The table of audio routing options for the Notepad-8FX:

    index     device label     descriptions from the manual
    0         MIC 1+2          Mic inputs 1+2
    1         LINE 3+4         Stereo input 3+4
    2         LINE 5+6         Stereo input 5+6
    3         MASTER L+R       Mix L+R

Table for Notepad-12FX sources:

    index     device label     descriptions from the manual
    0         MIC 3+4          Mic input 3+4
    1         LINE 5+6         Stereo input 5+6
    2         LINE 7+8         Stereo input 7+8
    3         MASTER L+R       Mix L+R

The audio routing command been tested to work with scnp-cli for the
Notepad-12FX, and with soundcraft-utils for Notepad-5,
Notepad-8FX. and Notepad-12FX.


Ducker
======

Note: The ducker information is still preliminary, and needs testing.

As the MacOS GUI only sends USB requests changing the settings of the
ducker while the ducker is actually enabled both in the GUI and on the
mixer, it might be prudent to do the same in other software
implementations.

CONTROL OUT message with endpoint 0 setting the "release" time,
enabling/disabling ducking and setting the input set to use for
ducking, all at the same time:

    00 00 02 80 01 01 0c 15
                ┗┩ ┗┩ ┗━━━┩
                 │  │     └── network endian release value in ms
                 │  │         (observed range is 0..5000)
                 │  └──────── bits[3..0] are INPUT[4..1] enable bitfield
                 └─────────── 0x01 = enable ducking, 0x00 disable ducking

TODO: What are those four inputs? Just the first four MIC channels? Or
whatever the four Audio capture channels are? And what about the 8FX
with its two MIC channels and two channel Audio capture? And what
about the Notepad-5 with its single MIC channel? Screenshots from the
Windows/MacOS vendor software with one of those devices connected
might be helpful here, even without packet dumps.

CONTROL OUT message with endpoint 0 setting the "Duck range":

    00 00 02 81 1f ff ff ff

    00 00 02 81 00 00 4a 67
                ┗━━━━━━━━━┩
                          └── network endian "duck range" value
                              observed range is 0x1fff_ffff to 0x0000_4a67
                              corresponding to displayed values of 0dB to 90dB

TODO: How exactly is the mapping from the 0dB .. 90dB range to the
      0x1fff_ffff .. 0x0000_4a67 (or perhaps even 0x0000_0000?) range?

CONTROL OUT message with endpoint 0 setting the "threshold":

    00 00 02 82 00 7f ff ff
                ┗━━━━━━━━━┩
                          └── network endian threshold value
                              observed range is 0x20c3=8387 to 0x7fffff=8388607
                              corresponding to displayed values of -60dB to 0dB

TODO: How exactly is the mapping from the -60dB .. 0dB range to the
      0x0020c3 (or perhaps even 0x000000?) .. 0x7fffff range?

TODO: How to observe the meter value?
