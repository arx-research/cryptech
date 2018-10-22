#!/bin/sh -

# pkcs11-tool's naming scheme for key types is buried in code.
# The useful choices in our case appear to be:
#
#  rsa:1024
#  rsa:2048
#  EC:prime256v1
#  EC:prime384v1

: ${key_type='EC:prime256v1'}

. ./environment.sh

pkcs11-tool --module ${PKCS11_MODULE} --login --pin ${PKCS11_PIN} --keypairgen --id 1 --label leader  --key-type "$key_type"
pkcs11-tool --module ${PKCS11_MODULE} --login --pin ${PKCS11_PIN} --keypairgen --id 2 --label boris   --key-type "$key_type"
pkcs11-tool --module ${PKCS11_MODULE} --login --pin ${PKCS11_PIN} --keypairgen --id 3 --label natasha --key-type "$key_type"
