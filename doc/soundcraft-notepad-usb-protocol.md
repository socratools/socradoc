The Soundcraft Notepad USB protocol
===================================

This document describes Soundcraft's Notepad series of mixers and how
the USB protocol the **Soundcraft USB audio control panel** and the
Soundcraft Notepad series mixer appears to work together.

The information in this document summarizes what could be observed,
and makes no guarantees whatsoever regarding

  * accuracy
  * applicability
  * usefulness
  * validity

USE the information from this document AT YOUR OWN RISK.

The Notepad series of mixers comprises the following three models:

  * Notepad-5    (2/2 USB class compliant audio device, no FX unit)
  * Notepad-8FX  (2/2 USB class compliant audio device)
  * Notepad-12FX (4/4 USB class compliant audio device)

Configurable audio routing appears to have been a device feature from
the start, as it already is described in the 1.0 version of the
manual.

According to a Soundcraft press release from Harman Professional from
January 24, 2018, the 1.09 firmware version has introduced the Ducker
feature for all three devices in the Notepad series.

Soundcraft provides the **Soundcraft USB audio control panel** which
allows changing the mixer settings for audio routing and the
ducker. However, that software is only available in binary form for
Windows and MacOS X, and the documentation provided by Soundcraft
provides no description of the USB protocol the **Soundcraft USB audio
control panel** uses to communicate with the mixer devices.


General packet format
=====================

Changing the mixer settings works by the USB host sending a 8 byte USB
control OUT request packet to the mixer's endpoint 0.  The control
requests we have observed all have the following properties:

    USB Control OUT transfer with
         0x40  bmRequestType
         16    bRequest
         0     wValue
         0     wIndex

    sends 8 byte command data buffer

     0  1  2  3  4  5  6  7
    00 00 02 81 00 00 4a 67
          ┗┩ ┗┩ ┗━━━━━━━━━┩
           │  │           └── parameter(s) for the specific command
           │  └────────────── command within command category
           └───────────────── command category

To receive the meter value, a very similar USB Control IN transfer is
used to receive 8 bytes of data:

    USB Control IN transfer with
         0xc0  bmRequestType
         16    bRequest
         0     wValue
         0     wIndex

    receives 8 byte of data buffer

Caution: While most of the multi byte values in the data buffer are
         big endian, some are little endian.


Mixer state vs host software state
==================================

We have not observed the **Soundcraft USB audio control panel** query
any information from the mixer.

On starting the "Soundcraft USB audio control panel", no USB requests
can be observed which query the mixer state. However, the "Soundcraft
USB audio control panel" remembers the last state it set the mixer to
when it is started the next time, which means that some software
component must keep track of the state the mixer has been set to.

When the mixer state is changed via USB commands, such as

  * non-default audio routing, the default being Mix L+R
  * enabled ducking
  * enabled ducking with non-default range and threshold values

and the mixer is power cycled, the mixer sets its state to reasonable
default values: audio routing is set to "Mix L+R", and ducking is off.

If you then re-enable ducking with a `ducker on/off` command, the range
and threshold appear to have been reset to reasonable default values.

This behaviour suggests that the mixer has no persistent storage for
any state. (Avoiding persistent state in the mixer device also makes
sense from device design, user experience, and user support
perspectives.)

There might still be a few "query mixer state" USB commands
implemented in the mixer, but we have not observed their use yet.


Audio Routing
=============

On all three devices in the Notepad series, you can change the audio
channels the mixer delivers as the USB capture device using the `audio
routing` command.


`audio routing` command
-----------------------

Using the `audio routing` command, you can select which of the mixer's
audio channels are routed to the USB capture device.

On the 2/2 channel USB capture devices Notepad-5 and Notepad-8FX, the
two selected channels go to the two channels of the USB capture
device. On the 4/4 channel USB capture device Notepad-12FX, you can
select the two mixer channels going to USB capture device channels
3+4, but the USB capture device channels 1+2 are always fixed to
MIC 1 + MIC 2.

     0  1  2  3  4  5  6  7
    00 00 04 00 00 00 00 00
          ┗┩    ┗┩
           │     └─────────── index into source table (range 0x00..0x03)
           └───────────────── 0x04 = audio routing command

    The unlabeled bytes have always been observed to be 0x00.

Table of possible audio sources for the Notepad-5:

    index     device label     description from the manual
    0         MIC 1 + LINE 2   Mic Input 1 + Mono line input 2
    1         LINE 2/3         Stereo Input 2+3
    2         LINE 4/5         Stereo Input 4+5
    3         MASTER L+R       Mix L+R

Table of possible audio sources for the Notepad-8FX:

    index     device label     descriptions from the manual
    0         MIC 1+2          Mic inputs 1+2
    1         LINE 3/4         Stereo input 3+4
    2         LINE 5/6         Stereo input 5+6
    3         MASTER L+R       Mix L+R

Table of possible audio sources for the Notepad-12FX:

    index     device label     descriptions from the manual
    0         MIC 3+4          Mic input 3+4
    1         LINE 5/6         Stereo input 5+6
    2         LINE 7/8         Stereo input 7+8
    3         MASTER L+R       Mix L+R

The `audio routing` command been tested to work with `scnp-cli` for
the Notepad-12FX, and with `socranop` for Notepad-5, Notepad-8FX. and
Notepad-12FX.


Ducker
======

The Notepad's ducker feature can duck the signal from the USB playback
channels 1+2 (on the Notepad-12FX four channel USB playback device,
USB playback channels 3+4 are not ducked) when it detects a signal on
any combination of the so-called "input" channels which are the
channels which are going to the USB audio capture device.

The ducker is not mentioned at all in the user manual for the Notepad
series of mixers, but it can be turned on and off and its parameters
tuned in the "Ducker" tab of Soundcraft's MacOS (and presumably
Windows) software for the Notepad series.

As the **Soundcraft USB audio control panel** only sends USB requests
changing the settings of the ducker while the ducker is actually
enabled, it appears to be prudent to do the same in any software
implementing commands to Notepad series mixers.

All ducker related commands appear to use the following format:

     0  1  2  3  4  5  6  7
    00 00 02 cc xx xx xx xx
          ┗┩ ┗┩ ┗━━━━━━━━━┩
           │  │           └── command specific parameters
           │  └────────────── ducker command (0x80, 0x81, 0x82)
           └───────────────── 0x02 = ducker related command

    The unlabeled bytes have always been observed to be 0x00.


`ducker on/off` command
-----------------------

The `ducker on/off` command not only enables/disables the ducker, but
it also configures the set of "input" channels the ducker is to
observe, and the ducker's release time:

     0  1  2  3  4  5  6  7
    00 00 02 80 01 01 0c 15
          ┗┩ ┗┩ ┗┩ ┗┩ ┗━━━┩
           │  │  │  │     └── release value in ms (network endian)
           │  │  │  │         (observed range is 0..5000)
           │  │  │  └──────── bits[3..0] are INPUT[4..1] enable bitfield
           │  │  └─────────── 0x01 = enable ducker, 0x00 disable ducker
           │  └────────────── 0x80 = ducker on/off command
           └───────────────── 0x02 = ducker related command

    The unlabeled bytes have always been observed to be 0x00.

On the Notepad-12FX, the four inputs we can select from are the same
four channels which are available on the USB capture device:

    0b0001  1  INPUT 1
    0b0010  2  INPUT 2
    0b0100  4  INPUT 3
    0b1000  8  INPUT 4

The Notepad-5 and Notepad-8FX have only two input channels on the USB
audio capture device, so the number of input selection options for the
ducker probably are two as well:

    0b0001  1  INPUT 1
    0b0010  2  INPUT 2

TODO: verify number of and encoding of input channels on NP-5, NP-8FX

In any case, the behaviour of the ducker very much depends on the last
`audio routing` command for the source of the signal to duck the USB
playback for.

There is a curiously weird edge case to be found here: When the audio
routing is set to 3 (MIX L+R), and the USB output channel 1+2 signal
is mixed into MIX L+R, the USB playback signal ducks itself!

TODO: What is a reasonable ms release value a GUI could start with?


`ducker range` command
----------------------

CONTROL OUT message with endpoint 0 setting the "duck range":

     0  1  2  3  4  5  6  7
    00 00 02 81 00 00 4a 67
          ┗┩ ┗┩ ┗━━━━━━━━━┩
           │  │           └── network endian "duck range" value
           │  │               observed range is 0x1fff_ffff to 0x0000_4a67
           │  │               corresponding to displayed values of 0dB to 90dB
           │  └────────────── 0x81 = ducker range command
           └───────────────── 0x02 = ducker related command

    The unlabeled bytes have always been observed to be 0x00.

To map the integer values to dB values, the following equation looks
plausible:

                             /  range_uint  \
    range_dB = - 20 * log10 |  ------------  |
                             \  0x1fffffff  /

With the inverse function mapping dB values to integer values being

                                 /  -range_dB  \
                                |  -----------  |
                                 \      20     /
    range_uint = 0x1fffffff * 10

TODO: What is a reasonable dB range value a GUI could start with?

### calculated value table for duck range ###

    __dB__  _uint32_t_  _uint32_t_
       0dB  0x1fffffff   536870911
      10dB  0x0a1e89b1   169773489
      20dB  0x03333333    53687091
      30dB  0x01030dc5    16977349
      40dB  0x0051eb85     5368709
      50dB  0x0019e7c7     1697735
      60dB  0x00083127      536871
      70dB  0x0002972d      169773
      80dB  0x0000d1b7       53687
      90dB  0x00004251       16977


`ducker threshold` command
--------------------------

CONTROL OUT message with endpoint 0 setting the "threshold":

     0  1  2  3  4  5  6  7
    00 00 02 82 00 7f ff ff
          ┗┩ ┗┩ ┗━━━━━━━━━┩
           │  │           └── network endian threshold value
           │  │               observed range is 0x20c3=8387 to 0x7fffff=8388607
           │  │               corresponding to displayed values of -60dB to 0dB
           │  └────────────── 0x82 = ducker threshold command
           └───────────────── 0x02 = ducker related command

    The unlabeled bytes have always been observed to be 0x00.

To map the integer values to dB values, the following equation looks
plausible:

                            /  thresh_uint  \
    thresh_dB = 20 * log10 |  -------------  |
                            \   0x007fffff  /

With the inverse function mapping dB values to integer values being

                                 /  thresh_dB  \
                                |  -----------  |
                                 \      20     /
    thresh_uint = 0x007fffff * 10

TODO: What is a reasonable dB threshold value a GUI could start with?

### calculated value table for ducker threshold ###

    __dB__  _uint32_t_  _uint32_t_
     -60dB  0x000020c5        8389
     -50dB  0x0000679f       26527
     -40dB  0x000147ae       83886
     -30dB  0x00040c37      265271
     -20dB  0x000ccccd      838861
     -10dB  0x00287a26     2652710
       0dB  0x007fffff     8388607


`ducker meter` query
--------------------

It appears that to show the meter bar graph in the GUI, the
**Soundcraft USB audio control panel** just issues a USB Ctrl IN
Request every 0.05s where the 8 byte reply buffer then contains the
meter value:

    USB Control IN transfer with
         0xc0  bmRequestType
         16    bRequest
         0     wValue
         0     wIndex

Weirdly enough, the reply data is the only place in the Notepad USB
protocol which uses a multibyte value in LITTLE ENDIAN.

     0  1  2  3  4  5  6  7
    bd 07 00 01 00 00 00 00
    ┗━━━━━━━━━┩
              └────────────── meter value in LITTLE ENDIAN!
                              observed range:
                                * 0x0000008e .. 0x010007bd (stereo input)
                                * 0x0000009e .. 0x00800000 (mono input)
                                * continuously 0x00000000 while ducker is off

    The unlabeled bytes have always been observed to be 0x00.

To map the integer values to dB values, the following equation looks
plausible:

                           /  meter_uint  \
    meter_dB = 20 * log10 |  ------------  |
                           \  0x01000000  /

TODO: Need confirmation from GUI of meter range in uint32 and dB.

With the inverse function mapping dB values to integer values being

                                 /  meter_dB  \
                                |  ----------  |
                                 \     20     /
    meter_uint = 0x01000000 * 10

For an integer range of `0x8e` .. `0x010007bd`, this corresponds to
`-101.449dB` .. `0.001dB`, so some value clamping to the interval
`-100.0dB` .. `0.0dB` appears to be in order.

While it is clear that the integer values do not significantly exceed
the dB range from `-100dB` to `0dB`, it *is* clear that it *does*, so
care needs to be taken to clamp the values to the applicable interval
of numbers.

Note that the meter feature only works while the ducker is enabled. If
the ducker is *not* enabled, all meter responses will be all 0x00, so
you will receive a `0x00000000` value for meter_uint which means the
dB conversion function will compute the logarithm of `0` which will
either compute as negative infinity, throw an exception, or similar.

So the rule to only issue ducker related commands while the ducker is
actually enabled definitively applies here.

### calculated value table for ducker meter ###

    __dB__  _uint32_t_  _uint32_t_
    -100dB  0x000000a8         168
     -90dB  0x00000213         531
     -80dB  0x0000068e        1678
     -70dB  0x000014b9        5305
     -60dB  0x00004189       16777
     -50dB  0x0000cf3e       53054
     -40dB  0x00028f5c      167772
     -30dB  0x0008186e      530542
     -20dB  0x0019999a     1677722
     -10dB  0x0050f44e     5305422
       0dB  0x01000000    16777216

Caution: We have observed meter_uint values **outside** the range this
         table shows, so any software needs to deal with that. See
         above for more details.
