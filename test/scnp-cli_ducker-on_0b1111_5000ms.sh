#!/bin/sh

if lsusb -d 05fc:0032 || lsusb -d 05fc:0031 || lsusb -d 05fc:0030
then
    ${SCNP_CLI-scnp-cli} ducker-on 0b1111 5000ms
else
    exit 77
fi
