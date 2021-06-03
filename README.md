scnp-cli
========

The `scnp-cli` utility is a simple CLI written in C which implements
the USB control commands described in
[soundcraft-notepad-usb-protocol.md](doc/soundcraft-notepad-usb-protocol.md)
to make help verify the protocol description is accurate.

[soundcraft-notepad-usb-protocol.md](doc/soundcraft-notepad-usb-protocol.md)
describes how to tell a [Soundcraft Notepad
series](https://www.soundcraft.com/en/product_families/notepad-series)
mixer device which audio channels to route to the USB capture device,
and whether and how to configure the Ducker for the USB playback.

The Soundcraft Notepad series consists of these three devices:

  * [Notepad-5](https://www.soundcraft.com/en/products/notepad-5)
  * [Notepad-8FX](https://www.soundcraft.com/en/products/notepad-8fx)
  * [Notepad-12FX](https://www.soundcraft.com/en/products/notepad-12fx)

If you want a nice GUI for controlling a Notepad mixer but Soundcraft
does not provide one for your computer, go over to
[soundcraft-utils](https://github.com/lack/soundcraft-utils) for a
nice GUI which even synchronizes between multiple instances of its GUI
and CLI. [soundcraft-utils](https://github.com/lack/soundcraft-utils)
is written in Python, uses a D-Bus service to provide a better user
experience across tool invocations, and uses Gtk3 for the GUI.

`scnp-cli` could also be helpful if you do not want to install the
whole Python software stack just to change the mixer's audio routing
to the USB device.


Runtime requirements
--------------------

Unless you have statically linked `scnp-cli`, you need a libc (glibc?)
and `libusb-1.0` for running `scnp-cli`.


Usage
-----

This is the output of `scnp-cli --help`. A more detailed description
is available in the `scnp-cli(1)` man page.

```
Usage: scnp-cli <command> <command_params...>

Gives command line access to the USB control commands for the Soundcraft
Notepad series of mixers to help verify the USB protocol description document.

Commands:

    --help     Print this usage message and exit.

    --version  Print version message and exit.

    audio-routing <NUM>
               Find a supported device, and set its audio sources.
               There must be exactly one supported device connected.
               The valid source numbers are specific to the device:

               NOTEPAD-5 channels 1+2 of 2-channel audio capture device
                 0  MIC+LINE 1+2
                 1  LINE 2+3
                 2  LINE 4+5
                 3  MASTER L+R

               NOTEPAD-8FX channels 1+2 of 2-channel audio capture device
                 0  MIC 1+2
                 1  LINE 3+4
                 2  LINE 5+6
                 3  MASTER L+R

               NOTEPAD-12FX channels 3+4 of 4-channel audio capture device
                 0  MIC 3+4
                 1  LINE 5+6
                 2  LINE 7+8
                 3  MASTER L+R

               Note that on the NOTEPAD-12FX 4-channel audio capture device,
               capture device channels 1+2 are always fed from mixer CH 1+2.

    ducker-off
               Turn the ducker off.

    ducker-on <INPUTS> <RELEASE>ms
               Turn the ducker on and set the following values:
                 INPUTS (range 0..15) is a bitmask for the input "selection"
                         0b0000 =  0 = no inputs watched
                         0b1111 = 15 = all four inputs watched
                 RELEASE (range 0..5000) is a the "release" time in ms
                         0 = release time 0, 5000 = release time 5000ms=5s

    ducker-range <HEX_VALUE>|<RANGE>dB
               Set the "duck range" to the given value.
               Valid range is 0dB to 90dB, or 0 to 0x1fffffff.
               It may be best to only use this while ducker is on.

    ducker-threshold <HEX_VALUE>|<THRESH>dB
               Set the "threshold" to the given value.
               Valid range is -60dB to 0dB, or 0x000000 to 0x7fffff.
               It may be best to only use this while ducker is on.

    meter
               Show the meter until you press Ctrl-C
```


Build Requirements
------------------

For building `scnp-cli`, you need at least

  * clang or gcc
  * make (GNU make is helpful)
  * `libusb-1.0.pc` (from OS package like `libusbx-devel` or `libusb-1.0-0-dev`)
  * pkg-config command (from pkg-config or pkgconf package)
  * strongly recommended: bash-completion

For building from a git source tree, you also need at least

  * git
  * automake
  * autoconf


Building
--------

When building from a git source tree (or one of the github "release"
tarballs (`*.tar.gz`) or zipfiles (`*.zip`), which I wholeheartedly
recommend to NOT use):

    autoreconf -vis .

Then you can run the usual build commands as if you had unpacked a
proper dist tarball (`*.tar.xz`):

    ./configure
    make
    make install

or for an out of source tree build

    mkdir _b && cd _b
    ../configure
    make
    make install

Note that you do not need to install `scnp-cli` to run it. Just run it
from the build dir if that suits you better than installing it.

Note that if you run `make check` or `make distcheck` and there is
actually a Notepad series mixer connected, one of the test cases will
write to the device and possibly change its settings.


Device permission setup using udev
==================================

`scnp-cli` writes to the mixer device by opening the device special
file connected to the mixer, which is called something like
`/dev/bus/usb/006/004` and by default cannot be written to by ordinary
users.

So we need to tell the system that the next time a mixer device is
plugged in, it should set the permissions of the device special file
such that an ordinary user can access it.

To help with that, we have two udev rules files:

  * 70-soundcraft-notepad.rules 
    Uses the `TAG+="uaccess"` mechanism to allow access users locally
    logged into a login session.

  * 80-soundcraft-notepad.rules 
    Sets the device file's group to `audio` to allow all group members
    access.

Check whether these udev rule files are suitable for your system
before installing them. You can very well have both installed at the
same time.

We have two special make targets to help with installing and
uninstalling the udev rules files to the well-known system directory
for udev rules `/etc/udev/rules.d` for local installations:

    sudo make install-udev-rules
    sudo make uninstall-udev-rules

If you package `scnp-cli` (e.g. as `*.deb` or `*.rpm`), I suggest you
change the usual `make install` command to install the udev rules
similar to the following command:

    make DESTDIR=... udevrulesdir=/usr/lib/udev/rules.d install install-udev-rules

In any case, if you have installed or uninstalled and udev rules, the
permissions of any existing device special files do not change
according to the new set of rules until you **unplug and re-plug the
USB cable** to your mixer device(s).
