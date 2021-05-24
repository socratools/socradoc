The Soundcraft Notepad USB protocol
===================================

This document describes the USB protocol spoken between the
"Soundcraft USB audio control panel" and a Soundcraft Notepad mixer.


Audio Routing
=============

The "Audio Routing" tab allows us to select any of the four different
selections for the audio input.

CONTROL message with EP etc.. FIXME

    00 00 04 00 00 00 00 00
                ┗┩
                 └─────────── source index (range 0..3)

Table for Notepad-12FX sources

    index   description
    0       Mic input 3+4
    1       Stereo input 5+6
    2       Stereo input 7+8
    3       Mix L+R


Ducker
======

As the MacOS GUI only sends allows changing the settings of the ducker
while the ducker is actually enabled, it might be prudent to do the
same.

CONTROL OUT message with endpoint 0 setting the "release" time,
enabling/disabling ducking and setting the input set to use for
ducking, all at the same time:

    00 00 02 80 01 01 0c 15
                ┗┩ ┗┩ ┗━━━┩
                 │  │     └── network endian release value in ms
                 │  │         (observed range is 0..5000)
                 │  └──────── bits[3..0] are INPUT[4..1] enable bitfield
                 └─────────── 0x01 = enable ducking, 0x00 disable ducking

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

