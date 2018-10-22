#!/bin/bash

set -e

echo "##########################################"
echo "# Flashing bootloader"
echo "##########################################"
./bin/flash-target firmware/bootloader

echo "##########################################"
echo "# Flashing temporary cli-test HSM firmware"
echo "##########################################"
./bin/flash-target firmware/cli-test
./bin/reset

echo "##########################################"
echo "# Sleeping after reset"
echo "##########################################"
sleep 5

echo "##########################################"
echo "# Uploading bitstream"
echo "##########################################"

./bin/cryptech_upload --fpga --username ct

echo "##########################################"
echo "# Programming AVR"
echo "##########################################"
avrdude -c usbtiny -p attiny828 -U flash:w:firmware/tamper.hex

echo "##########################################"
echo "# Flashing official STM32 firmware"
echo "##########################################"
./bin/cryptech_upload --firmware --username ct
./bin/reset
sleep 15

echo "##########################################"
echo "# Checking FPGA"
echo "##########################################"
./bin/cryptech_runcmd --username wheel "fpga show cores"

