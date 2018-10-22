#!/bin/sh

P11SPEED=p11speed
MODULE=/usr/local/homebrew/lib/libcryptech-pkcs11.dylib

# use pkcs11spy for testing
export PKCS11SPY=$MODULE
#export PKCS11SPY_OUTPUT=pkcs11spy.log
MODULE=/Library/OpenSC/lib/pkcs11/pkcs11-spy.so

SLOT=0
USER_PIN=1234

THREADS=2

run_p11speed() {
	$P11SPEED \
		--sign \
		--module $MODULE \
		--slot $SLOT \
		--threads $THREADS \
		--pin $USER_PIN \
		$@
}

run_p11speed --mechanism RSA_PKCS --keysize 1024 --iterations 40
run_p11speed --mechanism RSA_PKCS --keysize 2048 --iterations 20
run_p11speed --mechanism RSA_PKCS --keysize 4096 --iterations 10

run_p11speed --mechanism DSA --keysize 1024 --iterations 40
run_p11speed --mechanism DSA --keysize 2048 --iterations 20
run_p11speed --mechanism DSA --keysize 4096 --iterations 10

run_p11speed --mechanism ECDSA --keysize 256 --iterations 20
run_p11speed --mechanism ECDSA --keysize 384 --iterations 10
