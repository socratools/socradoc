The Soundcraft Notepad USB protocol
===================================

This document describes the USB protocol spoken between the
"Soundcraft USB audio control panel" and a Soundcraft Notepad mixer
and the mixer state that protocol communicates about.


General packet format
=====================

The observed USB control OUT request packets all have these
properties:

    USB Control OUT transfer with
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
normal operation.

On starting the "Soundcraft USB audio control panel", no USB requests
can be observed which query the mixer state. However, the "Soundcraft
USB audio control panel" remembers the last state it set the mixer to
when it is started the next time, which means that some software
component must keep track of the state the mixer has been set to.

When the mixer state is changed via USB commands, such as

  * non-default audio routing, the default being Mix L+R
  * enabled ducking
  * enabled ducking with weird specific range and threshold values

and the mixer is power cycled, the audio routing is back to the "Mix
L+R" default, ducking is off, and if you then re-enable ducking, the
range and threshold appear set to reasonable default values.  This
suggests that the mixer has no nonvolatile storage for any state.

The question is still open, though, about whether there might be a
"query state" USB command for when a continuously powered mixer is
plugged into and unplugged from a computer repeatedly.


Audio Routing
=============

The "Audio Routing" tab of the MacOS control panel allows us to select
any one of four different options for the audio input.


audio-routing command
---------------------

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

The audio routing command been tested to work with `scnp-cli` for the
Notepad-12FX, and with `soundcraft-utils` for Notepad-5,
Notepad-8FX. and Notepad-12FX.


Ducker
======

The Notepad-12FX has a ducker feature which can duck USB playback
channels 1+2 (on the Notepad-12FX four channel USB playback device,
playback channel 3+4 are not ducked) when it detects a signal on any
combination of the four channels going to the USB audio capture
device.

TODO: What about the 8FX with its two MIC channels and two channel
      Audio capture? What about the Notepad-5 with its single MIC
      channel, two channel Audio interface and no FX engine?
      Screenshots from the Windows/MacOS vendor software with one of
      those devices connected might be helpful here, even without
      packet dumps.

The ducker is not mentioned at all in the user manual for the Notepad
series of mixers, but it can be turned on and off and its parameters
tuned in the "Ducker" tab of Soundcraft's MacOS (and presumably
Windows) software.

Note: The ducker information is still preliminary, and needs testing,
      especially on devices other than a Notepad-12FX, if those other
      devices even support the ducker feature.

The following command (`play` is part of the sox sound file conversion
tool) can be helpful for testing the ducker:

    play -n synth whitenoise

As the MacOS GUI only sends USB requests changing the settings of the
ducker while the ducker is actually enabled both in the GUI and on the
mixer, it might be prudent to do the same in other software
implementations.


ducker-off and ducker-on command
--------------------------------

CONTROL OUT message setting the "release" time, enabling/disabling the
ducker and configuring the set of inputs for the ducker, all at the
same time:

    00 00 02 80 01 01 0c 15
                ┗┩ ┗┩ ┗━━━┩
                 │  │     └── network endian release value in ms
                 │  │         (observed range is 0..5000)
                 │  └──────── bits[3..0] are INPUT[4..1] enable bitfield
                 └─────────── 0x01 = enable ducker, 0x00 disable ducker

The four (in the case of the Notepad-12FX) inputs we can select from
are the same four channels of the USB audio capture device.

    0001  1  INPUT 1
    0010  2  INPUT 2
    0100  4  INPUT 3
    1000  8  INPUT 4

This means that the ducker behaviour of the mixer depends on the last
"Audio Routing" command. And there is a curiously weird edge case
here: When the Audio Routing is set to 3 (Mix L+R), the USB output
channel 1+2 signal is mixed into MIX L+R, and thus ducks itself!


ducker-range command
--------------------

CONTROL OUT message with endpoint 0 setting the "Duck range":

    00 00 02 81 00 00 4a 67
                ┗━━━━━━━━━┩
                          └── network endian "duck range" value
                              observed range is 0x1fff_ffff to 0x0000_4a67
                              corresponding to displayed values of 0dB to 90dB

TODO: How exactly does the 0dB .. 90dB range map to the 0x1fff_ffff
      .. 0x0000_4a67 (or perhaps even 0x0000_0000?) range and back?


ducker-threshold command
------------------------

CONTROL OUT message with endpoint 0 setting the "threshold":

    00 00 02 82 00 7f ff ff
                ┗━━━━━━━━━┩
                          └── network endian threshold value
                              observed range is 0x20c3=8387 to 0x7fffff=8388607
                              corresponding to displayed values of -60dB to 0dB

TODO: How exactly does -60dB .. 0dB range map to the 0x0020c3 (or
      perhaps even 0x000000?) .. 0x7fffff range and back?


meter
-----

It appears that the vendor software just issues a USB Ctrl IN Request
every 0.05s where the 8 byte reply buffer then contains the meter
value:

    USB Control IN transfer with
         0xc0  bmRequestType
         16    bRequest
         0     wValue
         0     wIndex

Weirdly enough, the reply data appear to contain a LITTLE ENDIAN 32bit
value:

    bd 07 00 01 00 00 00 00
    ┗━━━━━━━━━┩
              └────────────── meter value in LITTLE ENDIAN!
                              observed range: 0x0000008e .. 0x010007bd
                              continuously 0x00000000 while is ducker off

TODO: How exactly does the integer value range 0x000000 .. 0x010007bd
      map to the meter dB value range the vendor GUI shows?
