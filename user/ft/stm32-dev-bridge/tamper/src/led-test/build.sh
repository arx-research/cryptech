#!/bin/bash

set -e

avr-gcc -mmcu=avr25 main.c -o main.elf
avr-objcopy -O ihex main.elf main.hex
sudo avrdude -c usbtiny -p attiny828 -U flash:w:main.hex
