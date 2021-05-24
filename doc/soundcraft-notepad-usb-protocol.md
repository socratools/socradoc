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
                 └───────────── source index (range 0..3)

Table for Notepad-12FX sources

    index   description
    0       Mic input 3+4
    1       Stereo input 5+6
    2       Stereo input 7+8
    3       Mix L+R


Ducker
======

CONTROL message with endpoint etc like the "Audio Routing" message.

    00 00 02 80 01 01 0c 15
                ┗┩ ┗┩
                 │  └────────── bits[3..0] are INPUT[4..1] enable bitfield
                 └───────────── 0x01 = enable ducking, 0x00 disable ducking

TODO: How to set the values for these:

  * Threshold (-60dB .. 0dB)
  * Duck range (0dB .. 90dB)
  * Release (0ms .. 5s)

TODO: How to observe the meter value?

