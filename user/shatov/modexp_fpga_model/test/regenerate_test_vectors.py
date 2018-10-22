#
# regenerate_test_vectors.py
# ------------------------------------------------------
# Generates a new test vectors set for modexp_fpga_model
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
# This script generates a new pair of test vectors. Every test vector contains
# public and private keypair and a demo message.
#

#
# imports
#
import time
import hashlib
import subprocess

if __name__ == "__main__":

		# generate two new keys
	subprocess.call(["openssl", "genrsa", "-out", "384.key",  "384"])
	subprocess.call(["openssl", "genrsa", "-out", "512.key",  "512"])

		# get current date and time
	dt_date = time.strftime("%Y-%m-%d")
	dt_time = time.strftime("%H:%M:%S")

		# format demo message
	msg = dt_date + " " + dt_time;
	print("msg = '" + msg + "'")

		# get two hashes of different lengths
	m384 = hashlib.sha384(msg.encode("utf-8")).hexdigest()
	m512 = hashlib.sha512(msg.encode("utf-8")).hexdigest()
	
		# zero out the leftmost byte of each hash to make sure, that its
		# numerical value is smaller than the corresponding modulus
	m384 = "00" + m384[2:]
	m512 = "00" + m512[2:]

		# save 384-bit message
	with open("384.txt", "w") as file_txt:
		file_txt.write(m384)
		
		# save 512-bit message
	with open("512.txt", "w") as file_txt:
		file_txt.write(m512)
		
		# everything went just fine
	print("New random test vectors generated.")

#
# End of file
#
