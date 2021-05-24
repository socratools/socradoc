scnp-cli
========

A simple CLI written in C for selecting the mixer channelsof the
[Soundcraft Notepad
series](https://www.soundcraft.com/en/product_families/notepad-series)
of mixer to connect to the USB audio capture device:

  * [Notepad-5](https://www.soundcraft.com/en/products/notepad-5)
  * [Notepad-8FX](https://www.soundcraft.com/en/products/notepad-8fx)
  * [Notepad-12FX](https://www.soundcraft.com/en/products/notepad-12fx)

If you want a nice GUI for that, go over to
[soundcraft-utils](https://github.com/lack/soundcraft-utils) for a
nice GUI which even synchronizes between multiple instances of its GUI
and CLI. [soundcraft-utils](https://github.com/lack/soundcraft-utils)
is written in Python, uses a D-Bus service to provide a better user
experience across tool invocations, and uses Gtk3 for the GUI.

`scnp-cli` could be helpful if you do not want to install the whole
Python software stack just to change the mixer's audio routing to the
USB device.


Runtime requirements
--------------------

Unless you have statically linked `scnp-cli`, you need a libc (glibc?)
and `libusb-1.0` for running `scnp-cli`.


Usage
-----

This is the output of `scnp-cli --help`:

```
Usage: scnp-cli <command>

Select mixer channels to connect the to USB audio capture device
of a Soundcraft Notepad series mixer.

Commands:

    --help     Print this usage message and exit.

    --version  Print version message and exit.

    set <NUM>  Find a supported device, and set its audio sources.
               There must be exactly one supported device connected.
               The valid source numbers are specific to the device:

               NOTEPAD-5 channels 1+2 of 2-channel audio capture device
                 0  CH 1+2
                 1  ST 2+3
                 2  ST 4+5
                 3  MASTER L+R

               NOTEPAD-8FX channels 1+2 of 2-channel audio capture device
                 0  CH 1+2
                 1  ST 3+4
                 2  ST 5+6
                 3  MASTER L+R

               NOTEPAD-12FX channels 3+4 of 4-channel audio capture device
                 0  CH 3+4
                 1  ST 5+6
                 2  ST 7+8
                 3  MASTER L+R

               Note that on the NOTEPAD-12FX 4-channel audio capture device,
               capture device channels 1+2 are always fed from mixer CH 1+2.

Example:

    [user@host ~]$ scnp-cli set 2
    Bus 006 Device 003: ID 05fc:0032 Soundcraft Notepad-12FX
    Setting USB audio source to 2 (ST 7+8) for device NOTEPAD-12FX
    [user@host ~]$ 
```


Build Requirements
------------------

For building `scnp-cli`, you need at least

  * clang or gcc
  * make
  * libusb-1.0 (might be in the libusbx OS package)
  * pkg-config or pkgconf

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
