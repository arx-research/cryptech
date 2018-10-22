#!/bin/sh -

. ./environment.sh

for label in leader boris natasha
do
    for type in privkey pubkey
    do
	pkcs11-tool --module ${PKCS11_MODULE} --login --pin ${PKCS11_PIN} --delete-object --label "$label" --type "$type"
    done
done
