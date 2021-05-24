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
nice GUI which even synchronizes between multiple
instances. [soundcraft-utils](https://github.com/lack/soundcraft-utils)
is written in Python and uses a D-Bus service to provide a better user
experience across tool invocations.

`scnp-cli` could be helpful if you do not want to install the whole
Python software stack just to change the mixer's audio routing to the
USB device.


Runtime requirements
--------------------

Unless you have statically linked `scnp-cli`, you need only a libc and
`libusb-1.0` for running `scnp-cli`.


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

When building from a git source tree:

    autoreconf -vis .

Then you can run the usual build commands

    ./configure
    make
    make install

or for an out of source tree build

    mkdir _b && cd _b
    ../configure
    make
    make install
