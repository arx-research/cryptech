#!/usr/bin/env python

"""
Time libhal RSA key generation
"""

from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
from datetime import datetime, timedelta

from cryptech.libhal import *

parser = ArgumentParser(description = __doc__, formatter_class = ArgumentDefaultsHelpFormatter)
parser.add_argument("-i", "--iterations", default = 100, type = int,    help = "iterations")
parser.add_argument("-p", "--pin",        default = "fnord",            help = "user PIN")
parser.add_argument("-t", "--token",      action  = "store_true",       help = "store key on token")
parser.add_argument("-k", "--keylen",     default = 2048, type = int,   help = "key length")
args = parser.parse_args()

hsm = HSM()
hsm.login(HAL_USER_NORMAL, args.pin)

flags = HAL_KEY_FLAG_USAGE_DIGITALSIGNATURE | (HAL_KEY_FLAG_TOKEN if args.token else 0)
sum   = timedelta()

for n in xrange(1, args.iterations + 1):

    t0 = datetime.now()

    k  = hsm.pkey_generate_rsa(args.keylen, flags)

    t1 = datetime.now()

    k.delete()

    sum += t1 - t0

    print "{:4d} this {} mean {}".format(n, t1 - t0, sum / n)
