#!/bin/sh -

. ./environment.sh

pkcs11-tool --module ${PKCS11_MODULE} --login --pin ${PKCS11_PIN} --list-objects 2>/dev/null
