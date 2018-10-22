#!/bin/sh -

. ./environment.sh

set -x

openssl dgst -sha256 -keyform ENGINE -engine pkcs11 -sign label_boris -out message.sig message.txt

openssl verify -CAfile leader.cer boris.cer

openssl x509 -noout -in boris.cer -pubkey |
openssl dgst -sha256 -verify /dev/stdin -signature message.sig message.txt
