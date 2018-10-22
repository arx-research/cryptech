#
# regenerate_test_vectors.py
# -------------------------------------------------------------------
# Generates a new set of randomized test vectors for ecdsa_fpga_model
#
# Author: Pavel Shatov
# Copyright (c) 2017, NORDUnet A/S
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# - Neither the name of the NORDUnet nor the names of its contributors may
#   be used to endorse or promote products derived from this software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#
# This script generates a pair of test vectors. Each test vector contains two
# private keys. One test vector is for P-256, the other one is for P-384.
#

#
# imports
#
import subprocess

CURVE_P256 = "prime256v1"
CURVE_P384 = "secp384r1"

SECRET_ALICE_256 = "alice_p256.key"
SECRET_ALICE_384 = "alice_p384.key"

SECRET_BOB_256 = "bob_p256.key"
SECRET_BOB_384 = "bob_p384.key"

def openssl_ecparam_genkey(curve, file):
	subprocess.call(["openssl", "ecparam", "-genkey", "-name", curve, "-out", file])

if __name__ == "__main__":

		# generate two new private keys for P-256 curve
	openssl_ecparam_genkey(CURVE_P256, SECRET_ALICE_256)
	openssl_ecparam_genkey(CURVE_P256, SECRET_BOB_256)
	
		# generate two new private keys for P-384 curve
	openssl_ecparam_genkey(CURVE_P384, SECRET_ALICE_384)
	openssl_ecparam_genkey(CURVE_P384, SECRET_BOB_384)

#
# End of file
#
