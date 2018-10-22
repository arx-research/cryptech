#!/bin/bash

set -e

echo "##########################################"
echo "# Flashing cli-test HSM firmware"
echo "##########################################"
./bin/flash-target firmware/cli-test
./bin/reset
sleep 5

echo "##########################################"
echo "# Checking FPGA"
echo "##########################################"
./bin/cryptech_runcmd --username ct "show fpga cores"

echo "##########################################"
echo "# Testing SDRAM FMC bus"
echo "##########################################"
./bin/cryptech_runcmd --username ct --timeout 20 "test sdram"

echo "##########################################"
echo "# Uploading FMC test bitstream"
echo "##########################################"
./bin/cryptech_upload --fpga --username ct -i firmware/alpha_fmc_top.bit

echo "##########################################"
echo "# Testing FPGA FMC bus"
echo "##########################################"
./bin/cryptech_runcmd --username ct --timeout 20 "test fmc"

