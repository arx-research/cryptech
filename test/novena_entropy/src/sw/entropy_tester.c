/* 
 * hash_tester.c
 * --------------
 * This program sends several commands to the coretest_hashes subsystem
 * in order to verify the SHA-1, SHA-256 and SHA-512/x hash function
 * cores.
 *
 * Note: This version of the program talks to the FPGA over an I2C bus.
 *
 * The single and dual block test cases are taken from the
 * NIST KAT document:
 * http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA_All.pdf
 *
 * 
 * Authors: Joachim Strömbergson, Paul Selkirk
 * Copyright (c) 2014, SUNET
 * 
 * Redistribution and use in source and binary forms, with or 
 * without modification, are permitted provided that the following 
 * conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in 
 *    the documentation and/or other materials provided with the 
 *    distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <ctype.h>

/* I2C configuration */
#define I2C_dev  "/dev/i2c-2"
#define I2C_addr 0x0f

/* command codes */
#define SOC       0x55
#define EOC       0xaa
#define READ_CMD  0x10
#define WRITE_CMD 0x11
#define RESET_CMD 0x01

/* response codes */
#define SOR       0xaa
#define EOR       0x55
#define READ_OK   0x7f
#define WRITE_OK  0x7e
#define RESET_OK  0x7d
#define UNKNOWN   0xfe
#define ERROR     0xfd

/* addresses and codes common to all hash cores */
#define ADDR_NAME0              0x00
#define ADDR_NAME1              0x01
#define ADDR_VERSION            0x02
#define ADDR_CTRL               0x08
#define CTRL_INIT_CMD           1
#define CTRL_NEXT_CMD           2
#define ADDR_STATUS             0x09
#define STATUS_READY_BIT        0
#define STATUS_VALID_BIT        1

/* addresses and codes for the specific hash cores */
#define SHA1_ADDR_PREFIX        0x10
#define SHA1_ADDR_BLOCK         0x10
#define SHA1_BLOCK_LEN          16
#define SHA1_ADDR_DIGEST        0x20
#define SHA1_DIGEST_LEN         5

#define SHA256_ADDR_PREFIX      0x20
#define SHA256_ADDR_BLOCK       0x10
#define SHA256_BLOCK_LEN        16
#define SHA256_ADDR_DIGEST      0x20
#define SHA256_DIGEST_LEN       8

#define SHA512_ADDR_PREFIX      0x30
#define SHA512_CTRL_MODE_LOW    2
#define SHA512_CTRL_MODE_HIGH   3
#define SHA512_ADDR_BLOCK       0x10
#define SHA512_BLOCK_LEN        32
#define SHA512_ADDR_DIGEST      0x40
#define SHA512_DIGEST_LEN       16
#define MODE_SHA_512_224        0
#define MODE_SHA_512_256        1
#define MODE_SHA_384            2
#define MODE_SHA_512            3

int i2cfd;
int debug = 0;

/* SHA-1/SHA-256 One Block Message Sample
   Input Message: "abc" */
const uint32_t NIST_512_SINGLE[] =
{ 0x61626380, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000018 };

const uint32_t SHA1_SINGLE_DIGEST[] =
{ 0xa9993e36, 0x4706816a, 0xba3e2571, 0x7850c26c,
  0x9cd0d89d };

const uint32_t SHA256_SINGLE_DIGEST[] =
{ 0xBA7816BF, 0x8F01CFEA, 0x414140DE, 0x5DAE2223,
  0xB00361A3, 0x96177A9C, 0xB410FF61, 0xF20015AD };

/* SHA-1/SHA-256 Two Block Message Sample
   Input Message: "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" */
const uint32_t NIST_512_DOUBLE0[] =
{ 0x61626364, 0x62636465, 0x63646566, 0x64656667,
  0x65666768, 0x66676869, 0x6768696A, 0x68696A6B,
  0x696A6B6C, 0x6A6B6C6D, 0x6B6C6D6E, 0x6C6D6E6F,
  0x6D6E6F70, 0x6E6F7071, 0x80000000, 0x00000000 };
const uint32_t NIST_512_DOUBLE1[] =
{ 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x000001C0 };

const uint32_t SHA1_DOUBLE_DIGEST[] =
{ 0x84983E44, 0x1C3BD26E, 0xBAAE4AA1, 0xF95129E5,
  0xE54670F1 };

const uint32_t SHA256_DOUBLE_DIGEST[] =
{ 0x248D6A61, 0xD20638B8, 0xE5C02693, 0x0C3E6039,
  0xA33CE459, 0x64FF2167, 0xF6ECEDD4, 0x19DB06C1 };

/* SHA-512 One Block Message Sample
   Input Message: "abc" */
const uint32_t NIST_1024_SINGLE[] =
{ 0x61626380, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000018 };

const uint32_t SHA512_224_SINGLE_DIGEST[] =
{ 0x4634270f, 0x707b6a54, 0xdaae7530, 0x460842e2,
  0x0e37ed26, 0x5ceee9a4, 0x3e8924aa };
const uint32_t SHA512_256_SINGLE_DIGEST[] =
{ 0x53048e26, 0x81941ef9, 0x9b2e29b7, 0x6b4c7dab,
  0xe4c2d0c6, 0x34fc6d46, 0xe0e2f131, 0x07e7af23 };
const uint32_t SHA384_SINGLE_DIGEST[] =
{ 0xcb00753f, 0x45a35e8b, 0xb5a03d69, 0x9ac65007,
  0x272c32ab, 0x0eded163, 0x1a8b605a, 0x43ff5bed,
  0x8086072b, 0xa1e7cc23, 0x58baeca1, 0x34c825a7 };
const uint32_t SHA512_SINGLE_DIGEST[] =
{ 0xddaf35a1, 0x93617aba, 0xcc417349, 0xae204131,
  0x12e6fa4e, 0x89a97ea2, 0x0a9eeee6, 0x4b55d39a,
  0x2192992a, 0x274fc1a8, 0x36ba3c23, 0xa3feebbd,
  0x454d4423, 0x643ce80e, 0x2a9ac94f, 0xa54ca49f };

/* SHA-512 Two Block Message Sample
   Input Message: "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
   "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu" */
const uint32_t NIST_1024_DOUBLE0[] =
{ 0x61626364, 0x65666768, 0x62636465, 0x66676869,
  0x63646566, 0x6768696a, 0x64656667, 0x68696a6b,
  0x65666768, 0x696a6b6c, 0x66676869, 0x6a6b6c6d,
  0x6768696a, 0x6b6c6d6e, 0x68696a6b, 0x6c6d6e6f,
  0x696a6b6c, 0x6d6e6f70, 0x6a6b6c6d, 0x6e6f7071,
  0x6b6c6d6e, 0x6f707172, 0x6c6d6e6f, 0x70717273,
  0x6d6e6f70, 0x71727374, 0x6e6f7071, 0x72737475,
  0x80000000, 0x00000000, 0x00000000, 0x00000000 };
const uint32_t NIST_1024_DOUBLE1[] =
{ 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000380 };

const uint32_t SHA512_224_DOUBLE_DIGEST[] = 
{ 0x23fec5bb, 0x94d60b23, 0x30819264, 0x0b0c4533,
  0x35d66473, 0x4fe40e72, 0x68674af9 };
const uint32_t SHA512_256_DOUBLE_DIGEST[] =
{ 0x3928e184, 0xfb8690f8, 0x40da3988, 0x121d31be,
  0x65cb9d3e, 0xf83ee614, 0x6feac861, 0xe19b563a };
const uint32_t SHA384_DOUBLE_DIGEST[] =
{ 0x09330c33, 0xf71147e8, 0x3d192fc7, 0x82cd1b47,
  0x53111b17, 0x3b3b05d2, 0x2fa08086, 0xe3b0f712,
  0xfcc7c71a, 0x557e2db9, 0x66c3e9fa, 0x91746039 };
const uint32_t SHA512_DOUBLE_DIGEST[] =
{ 0x8e959b75, 0xdae313da, 0x8cf4f728, 0x14fc143f,
  0x8f7779c6, 0xeb9f7fa1, 0x7299aead, 0xb6889018,
  0x501d289e, 0x4900f7e4, 0x331b99de, 0xc4b5433a,
  0xc7d329ee, 0xb6dd2654, 0x5e96e55b, 0x874be909 };

/* ---------------- I2C low-level code ---------------- */
int i2c_setup(char *dev, int addr)
{
    i2cfd = open(dev, O_RDWR);
    if (i2cfd < 0) {
	fprintf(stderr, "Unable to open %s: ", dev);
	perror("");
	i2cfd = 0;
	return 1;
    }

    if (ioctl(i2cfd, I2C_SLAVE, addr) < 0) {
	fprintf(stderr, "Unable to set I2C slave device 0x%02x: ", addr);
	perror("");
	return 1;
    }

    return 0;
}

int i2c_write(uint8_t *buf, int len)
{
    if (debug) {
	int i;
	printf("write [");
	for (i = 0; i < len; ++i)
	    printf(" %02x", buf[i]);
	printf(" ]\n");
    }

    if (write(i2cfd, buf, len) != len) {
	perror("i2c write failed");
	return 1;
    }

    return 0;
}

int i2c_read(uint8_t *b)
{
    /* read() on the i2c device only returns one byte at a time,
     * and tc_get_resp() needs to parse the response one byte at a time
     */
    if (read(i2cfd, b, 1) != 1) {
	perror("i2c read failed");
	return 1;
    }

    return 0;
}

/* ---------------- test-case low-level code ---------------- */
int tc_send_write_cmd(uint8_t addr0, uint8_t addr1, uint32_t data)
{
    uint8_t buf[9];

    buf[0] = SOC;
    buf[1] = WRITE_CMD;
    buf[2] = addr0;
    buf[3] = addr1;
    buf[4] = (data >> 24) & 0xff;
    buf[5] = (data >> 16) & 0xff;
    buf[6] = (data >> 8) & 0xff;
    buf[7] = data & 0xff;
    buf[8] = EOC;

    return i2c_write(buf, sizeof(buf));
}

int tc_send_read_cmd(uint8_t addr0, uint8_t addr1)
{
    uint8_t buf[5];

    buf[0] = SOC;
    buf[1] = READ_CMD;
    buf[2] = addr0;
    buf[3] = addr1;
    buf[4] = EOC;

    return i2c_write(buf, sizeof(buf));
}

int tc_get_resp(uint8_t *buf, int len)
{
    int i;

    for (i = 0; i < len; ++i) {
	if (i2c_read(&buf[i]) != 0)
	    return 1;
	if ((i == 0) && (buf[i] != SOR)) {
	    /* we've gotten out of sync, and there's probably nothing we can do */
	    fprintf(stderr, "response byte 0: expected 0x%02x (SOR), got 0x%02x\n",
		    SOR, buf[0]);
	    return 1;
	}
	else if (i == 1) {	/* response code */
	    switch (buf[i]) {
	    case READ_OK:
		len = 9;
		break;
	    case WRITE_OK:
		len = 5;
		break;
	    case RESET_OK:
		len = 3;
		break;
	    case ERROR:
	    case UNKNOWN:
		len = 4;
		break;
	    default:
		/* we've gotten out of sync, and there's probably nothing we can do */
		fprintf(stderr, "unknown response code 0x%02x\n", buf[i]);
		return 1;
	    }
	}
    }

    if (debug) {
	printf("read  [");
	for (i = 0; i < len; ++i)
	    printf(" %02x", buf[i]);
	printf(" ]\n");
    }

    return 0;
}

int tc_get_expected(uint8_t *expected, int len)
{
    uint8_t buf[9];
    int i;

    if (tc_get_resp(buf, sizeof(buf)) != 0)
	return 1;

    for (i = 0; i < len; ++i) {
	if (buf[i] != expected[i]) {
	    fprintf(stderr, "response byte %d: expected 0x%02x, got 0x%02x\n",
		    i, expected[i], buf[i]);
	    return 1;
	}
    }

    return 0;
}

int tc_get_write_resp(uint8_t addr0, uint8_t addr1)
{
    uint8_t expected[5];

    expected[0] = SOR;
    expected[1] = WRITE_OK;
    expected[2] = addr0;
    expected[3] = addr1;
    expected[4] = EOR;

    return tc_get_expected(expected, sizeof(expected));
}

int tc_get_read_resp(uint8_t addr0, uint8_t addr1, uint32_t data)
{
    uint8_t expected[9];

    expected[0] = SOR;
    expected[1] = READ_OK;
    expected[2] = addr0;
    expected[3] = addr1;
    expected[4] = (data >> 24) & 0xff;
    expected[5] = (data >> 16) & 0xff;
    expected[6] = (data >> 8) & 0xff;
    expected[7] = data & 0xff;
    expected[8] = EOR;

    return tc_get_expected(expected, sizeof(expected));
}

int tc_write(uint8_t addr0, uint8_t addr1, uint32_t data)
{
    return (tc_send_write_cmd(addr0, addr1, data) ||
	    tc_get_write_resp(addr0, addr1));
}

int tc_read(uint8_t addr0, uint8_t addr1, uint32_t data)
{
    return (tc_send_read_cmd(addr0, addr1) ||
	    tc_get_read_resp(addr0, addr1, data));
}

int tc_init(uint8_t addr0)
{
    return tc_write(addr0, ADDR_CTRL, CTRL_INIT_CMD);
}

int tc_next(uint8_t addr0)
{
    return tc_write(addr0, ADDR_CTRL, CTRL_NEXT_CMD);
}

int tc_wait(uint8_t addr0, uint8_t status)
{
    uint8_t buf[9];

    do {
	if (tc_send_read_cmd(addr0, ADDR_STATUS) != 0)
	    return 1;
	if (tc_get_resp(buf, 9) != 0)
	    return 1;
	if (buf[1] != READ_OK)
	    return 1;
    } while ((buf[7] & status) != status);

    return 0;
}

int tc_wait_ready(uint8_t addr0)
{
    return tc_wait(addr0, STATUS_READY_BIT);
}

int tc_wait_valid(uint8_t addr0)
{
    return tc_wait(addr0, STATUS_VALID_BIT);
}

/* ---------------- SHA-1 test cases ---------------- */

int sha1_read(uint8_t addr, uint32_t data)
{
    return tc_read(SHA1_ADDR_PREFIX, addr, data);
}

int sha1_write(uint8_t addr, uint32_t data)
{
    return tc_write(SHA1_ADDR_PREFIX, addr, data);
}

int sha1_init(void)
{
    return tc_init(SHA1_ADDR_PREFIX);
}

int sha1_next(void)
{
    return tc_next(SHA1_ADDR_PREFIX);
}

int sha1_wait_ready(void)
{
    return tc_wait_ready(SHA1_ADDR_PREFIX);
}

int sha1_wait_valid(void)
{
    return tc_wait_valid(SHA1_ADDR_PREFIX);
}

/* TC1: Read name and version from SHA-1 core. */
int TC1(void)
{
    uint32_t name0   = 0x73686131;	/* "sha1" */
    uint32_t name1   = 0x20202020;	/* "    " */
    uint32_t version = 0x302e3530;	/* "0.50" */

    printf("TC1: Reading name, type and version words from SHA-1 core.\n");

    return 
	sha1_read(ADDR_NAME0, name0) ||
	sha1_read(ADDR_NAME1, name1) ||
	sha1_read(ADDR_VERSION, version);
}

/* TC2: SHA-1 Single block message test as specified by NIST. */
int TC2(void)
{
    const uint32_t *block = NIST_512_SINGLE;
    const uint32_t *expected = SHA1_SINGLE_DIGEST;
    int i;

    printf("TC2: Single block message test for SHA-1.\n");

    /* Write block to SHA-1. */
    for (i = 0; i < SHA1_BLOCK_LEN; ++i) {
	if (sha1_write(SHA1_ADDR_BLOCK + i, block[i]) != 0)
	    return 1;
    }

    /* Start initial block hashing, wait and check status. */
    if ((sha1_init() != 0) || (sha1_wait_valid() != 0))
	return 1;

    /* Extract the digest. */
    for (i = 0; i < SHA1_DIGEST_LEN; ++i) {
	if (sha1_read(SHA1_ADDR_DIGEST + i, expected[i]) != 0)
	    return 1;
    }

    return 0;
}

/* TC3: SHA-1 Double block message test as specified by NIST. */
int TC3(void)
{
    const uint32_t *block[2] = { NIST_512_DOUBLE0, NIST_512_DOUBLE1 };
    static const uint32_t block0_expected[] =
	{ 0xF4286818, 0xC37B27AE, 0x0408F581, 0x84677148, 0x4A566572 };
    const uint32_t *expected = SHA1_DOUBLE_DIGEST;
    int i;

    printf("TC3: Double block message test for SHA-1.\n");

    /* Write first block to SHA-1. */
    for (i = 0; i < SHA1_BLOCK_LEN; ++i) {
	if (sha1_write(SHA1_ADDR_BLOCK + i, block[0][i]) != 0)
	    return 1;
    }

    /* Start initial block hashing, wait and check status. */
    if ((sha1_init() != 0) || (sha1_wait_valid() != 0))
	return 1;

    /* Extract the first digest. */
    for (i = 0; i < SHA1_DIGEST_LEN; ++i) {
	if (sha1_read(SHA1_ADDR_DIGEST + i, block0_expected[i]) != 0)
	    return 1;
    }

    /* Write second block to SHA-1. */
    for (i = 0; i < SHA1_BLOCK_LEN; ++i) {
	if (sha1_write(SHA1_ADDR_BLOCK + i, block[1][i]) != 0)
	    return 1;
    }

    /* Start next block hashing, wait and check status. */
    if ((sha1_next() != 0) || (sha1_wait_valid() != 0))
	return 1;

    /* Extract the second digest. */
    for (i = 0; i < SHA1_DIGEST_LEN; ++i) {
	if (sha1_read(SHA1_ADDR_DIGEST + i, expected[i]) != 0)
	    return 1;
    }

    return 0;
}

/* ---------------- SHA-256 test cases ---------------- */

int sha256_read(uint8_t addr, uint32_t data)
{
    return tc_read(SHA256_ADDR_PREFIX, addr, data);
}

int sha256_write(uint8_t addr, uint32_t data)
{
    return tc_write(SHA256_ADDR_PREFIX, addr, data);
}

int sha256_init(void)
{
    return tc_init(SHA256_ADDR_PREFIX);
}

int sha256_next(void)
{
    return tc_next(SHA256_ADDR_PREFIX);
}

int sha256_wait_ready(void)
{
    return tc_wait_ready(SHA256_ADDR_PREFIX);
}

int sha256_wait_valid(void)
{
    return tc_wait_valid(SHA256_ADDR_PREFIX);
}

/* TC4: Read name and version from SHA-256 core. */
int TC4(void)
{
    uint32_t name0     = 0x73686132;	/* "sha2" */
    uint32_t name1     = 0x2d323536;	/* "-256" */
    uint32_t version   = 0x302e3830;	/* "0.80" */

    printf("TC4: Reading name, type and version words from SHA-256 core.\n");

    return
	sha256_read(ADDR_NAME0, name0) ||
	sha256_read(ADDR_NAME1, name1) ||
	sha256_read(ADDR_VERSION, version);
}

/* TC5: SHA-256 Single block message test as specified by NIST. */
int TC5(void)
{
    const uint32_t *block = NIST_512_SINGLE;
    const uint32_t *expected = SHA256_SINGLE_DIGEST;
    int i;

    printf("TC5: Single block message test for SHA-256.\n");

    /* Write block to SHA-256. */
    for (i = 0; i < SHA256_BLOCK_LEN; ++i) {
	if (sha256_write(SHA256_ADDR_BLOCK + i, block[i]) != 0)
	    return 1;
    }

    /* Start initial block hashing, wait and check status. */
    if ((sha256_init() != 0) || (sha256_wait_valid() != 0))
	return 1;

    /* Extract the digest. */
    for (i = 0; i < SHA256_DIGEST_LEN; ++i) {
	if (sha256_read(SHA256_ADDR_DIGEST + i, expected[i]) != 0)
	    return 1;
    }


    return 0;
}

/* TC6: SHA-1 Double block message test as specified by NIST. */
int TC6(void)
{
    const uint32_t *block[2] = { NIST_512_DOUBLE0, NIST_512_DOUBLE1 };
    static const uint32_t block0_expected[] = 
	{ 0x85E655D6, 0x417A1795, 0x3363376A, 0x624CDE5C,
	  0x76E09589, 0xCAC5F811, 0xCC4B32C1, 0xF20E533A };
    const uint32_t *expected = SHA256_DOUBLE_DIGEST;
    int i;

    printf("TC6: Double block message test for SHA-256.\n");

    /* Write first block to SHA-256. */
    for (i = 0; i < SHA256_BLOCK_LEN; ++i) {
	if (sha256_write(SHA256_ADDR_BLOCK + i, block[0][i]) != 0)
	    return 1;
    }

    /* Start initial block hashing, wait and check status. */
    if ((sha256_init() != 0) || (sha256_wait_valid() != 0))
	return 1;

    /* Extract the first digest. */
    for (i = 0; i < SHA256_DIGEST_LEN; ++i) {
	if (sha256_read(SHA256_ADDR_DIGEST + i, block0_expected[i]) != 0)
	    return 1;
    }

    /* Write second block to SHA-256. */
    for (i = 0; i < SHA256_BLOCK_LEN; ++i) {
	if (sha256_write(SHA256_ADDR_BLOCK + i, block[1][i]) != 0)
	    return 1;
    }

    /* Start next block hashing, wait and check status. */
    if ((sha256_next() != 0) || (sha256_wait_valid() != 0))
	return 1;

    /* Extract the second digest. */
    for (i = 0; i < SHA256_DIGEST_LEN; ++i) {
	if (sha256_read(SHA256_ADDR_DIGEST + i, expected[i]) != 0)
	    return 1;
    }

    return 0;
}

/* TC7: SHA-256 Huge message test. */
int TC7(void)
{
    static const uint32_t block[] =
	{ 0xaa55aa55, 0xdeadbeef, 0x55aa55aa, 0xf00ff00f,
	  0xaa55aa55, 0xdeadbeef, 0x55aa55aa, 0xf00ff00f,
	  0xaa55aa55, 0xdeadbeef, 0x55aa55aa, 0xf00ff00f,
	  0xaa55aa55, 0xdeadbeef, 0x55aa55aa, 0xf00ff00f };

    /* final digest after 1000 iterations */
    static const uint32_t expected[] = 
	{ 0x7638f3bc, 0x500dd1a6, 0x586dd4d0, 0x1a1551af,
	  0xd821d235, 0x2f919e28, 0xd5842fab, 0x03a40f2a };

    int i, n = 1000;

    printf("TC7: Message with %d blocks test for SHA-256.\n", n);

    /* Write first block to SHA-256. */
    for (i = 0; i < SHA256_BLOCK_LEN; ++i) {
	if (sha256_write( SHA256_ADDR_BLOCK + i, block[i]) != 0)
	    return 1;
    }

    /* Start initial block hashing, wait and check status. */
    if ((sha256_init() != 0) || (sha256_wait_ready() != 0))
	return 1;

    /* First block done. Do the rest. */
    for (i = 1; i < n; ++i) {
	/* Start next block hashing, wait and check status. */
	if ((sha256_next() != 0) || (sha256_wait_ready() != 0))
	    return 1;
    }

    /* XXX valid is probably set at the same time as ready */
    if (sha256_wait_valid() != 0)
	return 1;
    /* Extract the final digest. */
    for (i = 0; i < SHA256_DIGEST_LEN; ++i) {
	if (sha256_read(SHA256_ADDR_DIGEST + i, expected[i]) != 0)
	    return 1;
    }

    return 0;
}

/* ---------------- SHA-512 test cases ---------------- */

int sha512_read(uint8_t addr, uint32_t data)
{
    return tc_read(SHA512_ADDR_PREFIX, addr, data);
}

int sha512_write(uint8_t addr, uint32_t data)
{
    return tc_write(SHA512_ADDR_PREFIX, addr, data);
}

int sha512_init(uint8_t mode)
{
    return tc_write(SHA512_ADDR_PREFIX, ADDR_CTRL,
		    CTRL_INIT_CMD + (mode << SHA512_CTRL_MODE_LOW));
}

int sha512_next(uint8_t mode)
{
    return tc_write(SHA512_ADDR_PREFIX, ADDR_CTRL,
		    CTRL_NEXT_CMD + (mode << SHA512_CTRL_MODE_LOW));
}

int sha512_wait_ready(void)
{
    return tc_wait_ready(SHA512_ADDR_PREFIX);
}

int sha512_wait_valid(void)
{
    return tc_wait_valid(SHA512_ADDR_PREFIX);
}

/* TC8: Read name and version from SHA-512 core. */
int TC8(void)
{
    uint32_t name0         = 0x73686132;	/* "sha2" */
    uint32_t name1         = 0x2d353132;	/* "-512" */
    uint32_t version       = 0x302e3830;	/* "0.80" */

    printf("TC8: Reading name, type and version words from SHA-512 core.\n");

    return 
	sha512_read(ADDR_NAME0, name0) ||
	sha512_read(ADDR_NAME1, name1) ||
	sha512_read(ADDR_VERSION, version);
}

/* TC9: SHA-512 Single block message test as specified by NIST.
   We do this for all modes. */
int tc9(uint8_t mode, const uint32_t *expected, int len)
{
    const uint32_t *block = NIST_1024_SINGLE;
    int i;

    /* Write block to SHA-512. */
    for (i = 0; i < SHA512_BLOCK_LEN; ++i) {
	if (sha512_write(SHA512_ADDR_BLOCK + i, block[i]) != 0)
	    return 1;
    }

    /* Start initial block hashing, wait and check status. */
    if ((sha512_init(mode) != 0) || (sha512_wait_valid() != 0))
	return 1;

    /* Extract the digest. */
    for (i = 0; i < len/4; ++i) {
	if (sha512_read(SHA512_ADDR_DIGEST + i, expected[i]) != 0)
	    return 1;
    }

    return 0;
}

int TC9(void)
{
    printf("TC9-1: Single block message test for SHA-512/224.\n");
    if (tc9(MODE_SHA_512_224, SHA512_224_SINGLE_DIGEST,
	    sizeof(SHA512_224_SINGLE_DIGEST)) != 0)
	return 1;

    printf("TC9-2: Single block message test for SHA-512/256.\n");
    if (tc9(MODE_SHA_512_256, SHA512_256_SINGLE_DIGEST,
	    sizeof(SHA512_256_SINGLE_DIGEST)) != 0)
	return 1;

    printf("TC9-3: Single block message test for SHA-384.\n");
    if (tc9(MODE_SHA_384, SHA384_SINGLE_DIGEST,
	    sizeof(SHA384_SINGLE_DIGEST)) != 0)
	return 1;

    printf("TC9-4: Single block message test for SHA-512.\n");
    if (tc9(MODE_SHA_512, SHA512_SINGLE_DIGEST,
	    sizeof(SHA512_SINGLE_DIGEST)) != 0)
	return 1;

    return 0;
}

/* TC10: SHA-512 Double block message test as specified by NIST.
   We do this for all modes. */
int tc10(uint8_t mode, const uint32_t *expected, int len)
{
    const uint32_t *block[2] = { NIST_1024_DOUBLE0, NIST_1024_DOUBLE1 };
    int i;

    /* Write first block to SHA-512. */
    for (i = 0; i < SHA512_BLOCK_LEN; ++i) {
	if (sha512_write(SHA512_ADDR_BLOCK + i, block[0][i]) != 0)
	    return 1;
    }

    /* Start initial block hashing, wait and check status. */
    if ((sha512_init(mode) != 0) || (sha512_wait_ready() != 0))
	return 1;

    /* Write second block to SHA-512. */
    for (i = 0; i < SHA512_BLOCK_LEN; ++i) {
	if (sha512_write(SHA512_ADDR_BLOCK + i, block[1][i]) != 0)
	    return 1;
    }

    /* Start next block hashing, wait and check status. */
    if ((sha512_next(mode) != 0) || (sha512_wait_valid() != 0))
	return 1;

    /* Extract the digest. */
    for (i = 0; i < len/4; ++i) {
	if (sha512_read(SHA512_ADDR_DIGEST + i, expected[i]) != 0)
	    return 1;
    }

    return 0;
}

int TC10(void)
{
    printf("TC10-1: Double block message test for SHA-512/224.\n");
    if (tc10(MODE_SHA_512_224, SHA512_224_DOUBLE_DIGEST,
	     sizeof(SHA512_224_DOUBLE_DIGEST)) != 0)
	return 1;

    printf("TC10-2: Double block message test for SHA-512/256.\n");
    if (tc10(MODE_SHA_512_256, SHA512_256_DOUBLE_DIGEST,
	     sizeof(SHA512_256_DOUBLE_DIGEST)) != 0)
	return 1;

    printf("TC10-3: Double block message test for SHA-384.\n");
    if (tc10(MODE_SHA_384, SHA384_DOUBLE_DIGEST,
	     sizeof(SHA384_DOUBLE_DIGEST)) != 0)
	return 1;

    printf("TC10-4: Double block message test for SHA-512.\n");
    if (tc10(MODE_SHA_512, SHA512_DOUBLE_DIGEST,
	     sizeof(SHA512_DOUBLE_DIGEST)) != 0)
	return 1;

    return 0;
}

/* ---------------- main ---------------- */

int main(int argc, char *argv[])
{
    typedef int (*tcfp)(void);
    tcfp all_tests[] = { TC1, TC2, TC3, TC4, TC5, TC6, TC7, TC8, TC9, TC10 };
    tcfp sha1_tests[] = { TC1, TC2, TC3 };
    tcfp sha256_tests[] = { TC4, TC5, TC6, TC7 };
    tcfp sha512_tests[] = { TC8, TC9, TC10 };

    char *usage = "Usage: %s [-d] [-i I2C_device] [-a I2C_addr] tc...\n";
    char *dev = I2C_dev;
    int addr = I2C_addr;
    int i, j, opt;

    while ((opt = getopt(argc, argv, "h?di:a:")) != -1) {
	switch (opt) {
	case 'h':
	case '?':
	    printf(usage, argv[0]);
	    return 0;
	case 'd':
	    debug = 1;
	    break;
	case 'i':
	    dev = optarg;
	    break;
	case 'a':
	    addr = (int)strtol(optarg, NULL, 0);
	    if ((addr < 0x03) || (addr > 0x77)) {
		fprintf(stderr, "addr must be between 0x03 and 0x77\n");
		return 1;
	    }
	    break;
	default:
	    fprintf(stderr, usage, argv[0]);
	    return 1;
	}
    }

    if (i2c_setup(dev, addr) != 0)
	return 1;

    /* no args == run all tests */
    if (optind >= argc) {
	for (j = 0; j < sizeof(all_tests)/sizeof(all_tests[0]); ++j)
	    if (all_tests[j]() != 0)
		return 1;
	return 0;
    }

    for (i = optind; i < argc; ++i) {
	if (strcmp(argv[i], "sha1") == 0) {
	    for (j = 0; j < sizeof(sha1_tests)/sizeof(sha1_tests[0]); ++j)
		if (sha1_tests[j]() != 0)
		    return 1;
	}
	else if (strcmp(argv[i], "sha256") == 0) {
	    for (j = 0; j < sizeof(sha256_tests)/sizeof(sha256_tests[0]); ++j)
		if (sha256_tests[j]() != 0)
		    return 1;
	}
	else if (strcmp(argv[i], "sha512") == 0) {
	    for (j = 0; j < sizeof(sha512_tests)/sizeof(sha512_tests[0]); ++j)
		if (sha512_tests[j]() != 0)
		    return 1;
	}
	else if (strcmp(argv[i], "all") == 0) {
	    for (j = 0; j < sizeof(all_tests)/sizeof(all_tests[0]); ++j)
		if (all_tests[j]() != 0)
		    return 1;
	}
	else if (isdigit(argv[i][0]) &&
		 (((j = atoi(argv[i])) > 0) &&
		  (j <= sizeof(all_tests)/sizeof(all_tests[0])))) {
	    if (all_tests[j - 1]() != 0)
		return 1;
	}
	else {
	    fprintf(stderr, "unknown test case %s\n", argv[i]);
	    return 1;
	}
    }

    return 0;
}
