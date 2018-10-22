#
# regenerate_test_vector.py
# ------------------------------------------------------------
# Generates a new randomized test vector for x25519_fpga_model
#
# Author: Pavel Shatov
# Copyright (c) 2017-2018, NORDUnet A/S
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
USAGE = "USAGE: regenerate_test_vector.py [openssl_binary]"
#

#
# This script generates a test vector. The test vector contains two
# private keys.
#

#
# IMPORTANT: This will only work with OpenSSL 1.1.0, version 1.0.2 doesn't
# support the X25519 algoritm. If you system library is the older version,
# you can pass location of specific OpenSSL binary to use on the command line.
#

#
# imports
#
import sys
import subprocess

#
# variables
#
SECRET_KEY_ALICE = "alice.key"
SECRET_KEY_BOB   = "bob.key"

OPENSSL = ""


#
# openssl_genpkey
#
def openssl_genpkey(binary, filename):
		
	subprocess.call([binary, "genpkey", "-algorithm", "X25519", "-out", filename])

	
#
# __main__
#
if __name__ == "__main__":

		# detect whether user requested some specific binary
	if len(sys.argv) == 1:
		OPENSSL = "openssl"
		print("Using system OpenSSL library.")
	elif len(sys.argv) == 2:
		OPENSSL = sys.argv[1]
		print("Using OpenSSL binary '" + OPENSSL + "'...")
	else:
		print(USAGE)
		
		# generate two new private keys
	if len(OPENSSL) > 0:
		openssl_genpkey(OPENSSL, SECRET_KEY_ALICE)
		openssl_genpkey(OPENSSL, SECRET_KEY_BOB)


#
# End of file
#
