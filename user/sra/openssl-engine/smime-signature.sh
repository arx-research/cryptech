#!/bin/sh -

. ./environment.sh

set -x

openssl smime -engine pkcs11 -sign -text -keyform ENGINE			\
	-inkey label_natasha -signer natasha.cer				\
	-from "Natasha Fatale <natasha@moo.pv>"					\
	-to   "Boris Badenov <boris@moo.pv>"					\
	-subject "Fiendish plot"						\
	-in message.txt -out message.smime

openssl smime -verify -in message.smime -CAfile leader.cer -out /dev/null
