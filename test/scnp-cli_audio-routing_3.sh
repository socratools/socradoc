#!/bin/sh

device_count="$(${LSUSB-lsusb} -d 05fc:0030 -d 05fc:0031 -d 05fc:0032 | ${WC-wc} -l)"
if test "x$device_count" = "x0"
then
    echo "No device found."
    exit 77
elif test "x$device_count" = "x1"
then
    ${SCNP_CLI-scnp-cli} audio-routing 3
else
    echo "Too many devices found."
    exit 77
fi
