#!/usr/bin/env python

"""
Time PKCS #11 signatures.
"""

import collections
import argparse
import datetime
import sys

from Crypto.Hash.SHA256 import SHA256Hash as SHA256
from Crypto.Hash.SHA384 import SHA384Hash as SHA384
from Crypto.Hash.SHA512 import SHA512Hash as SHA512

try:
    from cryptech.py11 import *
except ImportError:
    # Kludge to let us run script from repo without installing cryptech.py11
    from os.path import dirname, abspath
    sys.path.append(dirname(dirname(abspath(sys.argv[0]))))
    from cryptech.py11 import *

from cryptech.py11 import default_so_name as libpkcs11_default


class RSAKey(object):

    def __init__(self, hash, pem):
        from Crypto.PublicKey   import RSA
        self.h = hash
        self.k = RSA.importKey(pem)
        self.ckm = CKM_RSA_PKCS

    def create(self, args, p11, session, text):
        from Crypto.Util.number                 import inverse
        from Crypto.Signature                   import PKCS1_v1_5
        from Crypto.Signature.PKCS1_v1_5        import EMSA_PKCS1_V1_5_ENCODE, PKCS115_SigScheme
        self.handle = p11.C_CreateObject(
            session,
            CKA_SIGN            = True,
            CKA_TOKEN           = args.token,
            CKA_CLASS           = CKO_PRIVATE_KEY,
            CKA_KEY_TYPE        = CKK_RSA,
            CKA_MODULUS         = self.k.n,
            CKA_PUBLIC_EXPONENT = self.k.e,
            CKA_PRIVATE_EXPONENT= self.k.d,
            CKA_PRIME_1         = self.k.p,
            CKA_PRIME_2         = self.k.q,
            CKA_COEFFICIENT     = inverse(self.k.q, self.k.p),
            CKA_EXPONENT_1      = self.k.d % (self.k.p - 1),
            CKA_EXPONENT_2      = self.k.d % (self.k.q - 1))
        h = self.h(text)
        tbs = EMSA_PKCS1_V1_5_ENCODE(h, (self.k.n.bit_length() + 7) / 8)[2:]
        self.tbs = tbs[tbs.index("\x00") + 1:]
        self.sig = PKCS115_SigScheme(self.k).sign(h)

    def verify(self, sig):
        assert sig == self.sig


class ECDSAKey(object):

    def __init__(self, hash, pem):
        from ecdsa.keys import SigningKey
        self.h  = hash
        self.sk = SigningKey.from_pem(pem)
        self.vk = self.sk.get_verifying_key()
        self.ckm = CKM_ECDSA

    def create(self, args, p11, session, text):
        self.handle = p11.C_CreateObject(
            session,
            CKA_SIGN            = True,
            CKA_CLASS           = CKO_PRIVATE_KEY,
            CKA_KEY_TYPE        = CKK_EC,
            CKA_EC_PARAMS       = self.sk.curve.encoded_oid,
            CKA_VALUE           = self.sk.privkey.secret_multiplier)
        self.text = text
        self.tbs = self.h(text).digest()

    def verify(self, sig):
        assert self.vk.verify(sig, self.text, self.h)


def main():
    parser = argparse.ArgumentParser(description = __doc__,
                                     formatter_class = argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-i", "--iterations", default = 100, type = int,                help = "iterations")
    parser.add_argument("-p", "--pin",        default = "fnord",                        help = "user PIN")
    parser.add_argument("-s", "--slot",       default = 0, type = int,                  help = "slot number")
    parser.add_argument("-t", "--token",      action = "store_true",                    help = "store key on token")
    parser.add_argument("-l", "--libpkcs11",  default = libpkcs11_default,              help = "PKCS #11 library")
    parser.add_argument("-k", "--keys",       choices = tuple(key_table), nargs = "+",
                                              default = list(key_table),                help = "keys to test")
    parser.add_argument("-q", "--quiet",      action = "store_true",                    help = "be less chatty")
    args = parser.parse_args()

    p11 = PKCS11(args.libpkcs11)

    if not args.quiet:
        print "Initializing"

    p11.C_Initialize()
    session = p11.C_OpenSession(args.slot)
    p11.C_Login(session, CKU_USER, args.pin)

    for name in args.keys:

        print "Starting test with key {}, {} iterations".format(name, args.iterations)

        k = key_table[name]
        k.create(args, p11, session, "Your mother was a hamster")

        mean = datetime.timedelta(seconds = 0)

        for i in xrange(args.iterations):
            t0  = datetime.datetime.now()
            p11.C_SignInit(session, k.ckm, k.handle)
            sig = p11.C_Sign(session, k.tbs)
            t1  = datetime.datetime.now()
            k.verify(sig)
            if not args.quiet:
                print "{:4d} {}".format(i, t1 - t0)
            mean += t1 - t0

        mean /= args.iterations
        print "mean {}".format(mean)

        p11.C_DestroyObject(session, k.handle)

    p11.C_CloseAllSessions(args.slot)
    p11.C_Finalize()

key_table = collections.OrderedDict()

key_table.update(rsa_1024 = RSAKey(SHA256, '''\
-----BEGIN RSA PRIVATE KEY-----
MIICXQIBAAKBgQC95QlDOvlQhdCe/a7eIoX9SGPVfXfA8/62ilnF+NcwLrhxkr2R
4EVQB65+9AbxqM8Hqol6fhZzmDs48cl2tOFGJpE8iMhiFm4i6SGl2RXYaG0xi+lJ
FrXXCLFQovIEMuukBE129wra1xIB72tYR14lu8REX+Mhpbuz44M1jlCrlQIDAQAB
AoGAeU928l8bZIiH9PnlG318kYkMVhd4SGjXQK/zl9hXSC2goNV4i1d1kCHIJMwq
H3mTALe+aeVg3GnU85Tq+g2llzogoyXl8q902KbvImrM/XSbsue9/oj0OSgw+jKB
faFzX6FxAtNV5pmU9QiwauBIl/3yPCF9ifim5zg+pWCqLaECQQD59Z/R6TrTHxp6
w2vH4CJyP5KORcf+eMa50SAriMVBXsJzsBiLLVxKIZfWbQn9gytJqJZKmIHezZQm
dyam84fpAkEAwnvSF27RhxLXE037+t7k5MZti6BfNTeUBrwffteepL6qax9HK+h9
IQZ1vfNIqjZm8i7kQQyy4L8tRnk8mjZmzQJBAIUwfXWTilW+yBRMFx1M7+3itAv9
YODWqEWRCkxIN5tqi8CrP5jBleCmX8rRFTaxcxpvq42aD/GRp3SLntvs/ikCQCSg
GOKc1gyv+Z0DFK8cBtMmoz6mRwfInbHe/7dtd8zis0lVLJwSPm5Xvxi0ljyn3h9B
wW6Wq6Ezn50j+8u27wkCQQCcIFE01BDAdtFHtTJ3aaEM9IdMCYrcJ0I/Y0NTE2M6
lsTSiPyQjc4dQQJxFduvWHLx28bx+l7FTav7FaKntCJo
-----END RSA PRIVATE KEY-----
'''))

key_table.update(rsa_2048 = RSAKey(SHA256, '''\
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEAsbvq6syhDXD/OMVAuLoMceGQLUIiezfShVqFfyMqADjqhFRW
Wbonn0XV9ZkypU4Ib9n6PtLATNaefhpsUlI4s+20YTlQ7GiwJ9p97/N1o1u060ja
4LdqhfYtn8GZX+JAfa5NqpmLKCJ58XJ3q28MPLRwYp5yKckjkzchZHFyjs1W7r5a
JfeJ/vsQusX3klmCehJ1jxSHPh8o6lTjFMnBK8t360YTu0UGK/RUcEAYO7l7FWjd
8PjZfawXIrOAhCLkVvDFfpsl2oyFIL9d1QE87WdyyZXAtWLs62gnX+kiBq9gUhu5
GsgcQifHBcRiGZfH0TRIMgIsSjsorzHqJ9uoQwIDAQABAoIBAGqzx5/5A8NfEEpT
2bxNLcV8xqL1LmBNLh0TMEwYn1GM2fZh74lkwf7T3VTaCVbGlzgXZC4tNneq7XIF
iPyPEi2rSnyH/XZAj2kNukfBIOHW37HVhloco14TYmajwuGWomMRrtz521pYAF+c
+g042N7k8Qez2hQOBkaOdYSouz7RNdJUGUocRhcSkh+QZTBwtQxrkuhhHN+zkIri
+Q09hF2hAliHrh6mow8ci0gRsXnZzsdJfTX8CasHWTIll4gfrvWnUY7iYqB6ynRU
YN+7IgQXMUFLziIlH1qN+DlEYdznsgAPSS3JdTWh0cvjiO8wTFAnOIdsj+BpKoDB
PK2zzDkCgYEA3TP8h4Ds/y1tDijE3Sarrg0vWuY97sJmAla4qFHH4hscZ84NDzTM
I/ohLPJgpeR2MaBqZmVk9UFrd3rdt3/lX6kSO7Kffa9rVgfOB4CqJ4joso3j0qY8
V/iVBcDcD1h4aXCRX2tMJICUTgVU/N8/2wBEElcOUjZHGlcHmbHndlUCgYEAzbFm
ttPuIHrYHkmOm/DjYB65+WD8gfPMdeUbnx+yIt5xwimIbq1qWzl4++goTAAyaU/7
qM9IfveRue0B7yjnPa8fjN+CcXMGM6a3BIqeIv1icfgjHxlt7D+64FpENWXHvFE0
MhRliINfkTHm+U4+1s0045a+bLdTbfVly1gATDcCgYEAyOaoWmFL3k7hl1SLx9eR
YVj0Q3iNk0XX5BPjTmxIQCEjYVwRHFh1d897Rhk0kja26kepmypH0UADXNaofDqa
lpE10CZhGIOz1sTr6ICBCbscrN6VpgH5GGTa5AjPVNijNBBa1/DZjOWCzIGnOKuC
kWLicE3E4gIN/exBKOQdNqkCgYEAjA5PMg38BoGexoCvad8L81b4qqUvSg0HGv91
X1Plp3hvXRWKoFHUKWlox528UoOPz8V2ReteIZXQ1BhdSMtBKO8lPHa0CyuW/XR3
CdCY/Jorfg7HW1WlU0fRpxHPf8xdxAxGzhK1T86kM+kWrIpqnzf62zy5TK1HUYfW
WC8DhOECgYBzU8hIA0PU7aRPUs0o9MO9XcvVPvdX6UOKdNb9CnBMudS/chKHJUYP
d0fFAiVaRX0JMQ0RSrenxCqfWVtW3T3wFYNHB/IFRIUT3I44wwXJTNOeoi3FDTMx
EQfc0UFoFHyc3mYEKR4zHheqQG5OFBN89LqG3S+O69vc1qwCvNKL+Q==
-----END RSA PRIVATE KEY-----
'''))

key_table.update(rsa_4096 = RSAKey(SHA256, '''\
-----BEGIN RSA PRIVATE KEY-----
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
-----END RSA PRIVATE KEY-----
'''))

key_table.update(ecdsa_p256 = ECDSAKey(SHA256, '''\
-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIPBrVhD1iFF2e8wPkPf4N1038iR8xPgku/CVOT8lcSztoAoGCCqGSM49
AwEHoUQDQgAE3mB5BmN5Fa4fV74LsDWIpBUxktPqYGJ6WOBrjPs1HWkNU7JHO3qY
9yy+CXFSPb89GWQgb5wLtNPn4QYMj+KRTA==
-----END EC PRIVATE KEY-----
'''))

key_table.update(ecdsa_p384 = ECDSAKey(SHA384, '''\
-----BEGIN EC PRIVATE KEY-----
MIGkAgEBBDCVGo35Hbrf3Iys7mWRIm5yjg+6vPIgzbp2jCbDyszBo+wTxmQambG4
g8yocp4wM6+gBwYFK4EEACKhZANiAATYwa+M8T8jsNHKmMZTvPPflUIfrjuZZo1D
3kkkmN4r6cTNctjaeRdAfD0X40l4yPnGIP9ParuKVVl1y0TdQ7BS3g/Gj/LP33HD
ESP8gFDIKFCWSDX0uhmy+HsGsPwgNoY=
-----END EC PRIVATE KEY-----
'''))

key_table.update(ecdsa_p521 = ECDSAKey(SHA512, '''\
-----BEGIN EC PRIVATE KEY-----
MIHcAgEBBEIBtf+LKhJNQEJRFQ2cGQPcviwfp9IKSnD5EFTqAPtv7/+t2FtmdHHP
/fWIlZ7jcC5N9dWy6bKGu3+FqwgZAYLpsqigBwYFK4EEACOhgYkDgYYABADdfcUe
P0oAZQ5308v5ijcg4hePvhvVi+QKcrwmE9kirXCFoYN1tzPmXZmw8lNJenrbwaNz
opJR84LBHnomGPogAQGF0aRk0jE8w1j1oMfrrzV6vCWnkh7pyzsDnrLU1HrkWeqw
ihzwMzYJgFzToDH+fCh7nrBFZZZ9P9gPYMlSM5UMeA==
-----END EC PRIVATE KEY-----
'''))


if __name__ == "__main__":
    main()
