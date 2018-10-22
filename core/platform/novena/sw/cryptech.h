//======================================================================
//
// cryptech.h
// ----------
// Memory map and access functions for Cryptech cores.
//
//
// Authors: Joachim Strombergson, Paul Selkirk
// Copyright (c) 2015, NORDUnet A/S All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the name of the NORDUnet nor the names of its contributors may
//   be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//======================================================================

/*

Each Cryptech core has a set of 4-byte registers, which are accessed
through a 16-bit address. The address space is divided as follows:
  3 bits segment selector       | up to 8 segments
  5 bits core selector          | up to 32 cores/segment (see note below)
  8 bits register selector      | up to 256 registers/core (see modexp below)

i.e, the address is structured as:
sss ccccc rrrrrrrr

The I2C and UART communication channels use this 16-bit address format
directly in their read and write commands.

The EIM communications channel translates this 16-bit address into a
32-bit memory-mapped address in the range 0x08000000..807FFFF:
00001000000000 sss 0 ccccc rrrrrrrr 00

EIM, as implemented on the Novena, uses a 19-bit address space:
  Bits 18..16 are the semgent selector.
  Bits 15..10 are the core selector.
  Bits 9..2 are the register selector.
  Bits 1..0 are zero, because reads and writes are always word aligned.

Note that EIM can support 64 cores per segment, but we sacrifice one bit
in order to map it into a 16-bit address space.

*/


//------------------------------------------------------------------
// Default sizes
//------------------------------------------------------------------
#define CORE_SIZE               0x100


//------------------------------------------------------------------
// Addresses and codes common to all cores
//------------------------------------------------------------------
#define ADDR_NAME0              0x00
#define ADDR_NAME1              0x01
#define ADDR_VERSION            0x02
#define ADDR_CTRL               0x08
#define CTRL_INIT               1
#define CTRL_NEXT               2
#define ADDR_STATUS             0x09
#define STATUS_READY            1
#define STATUS_VALID            2


// a handy macro from cryptlib
#ifndef bitsToBytes
#define bitsToBytes(x)          (x / 8)
#endif


//------------------------------------------------------------------
// Board-level registers and communication channel registers
//------------------------------------------------------------------
#define BOARD_ADDR_NAME0        ADDR_NAME0
#define BOARD_ADDR_NAME1        ADDR_NAME1
#define BOARD_ADDR_VERSION      ADDR_VERSION
#define BOARD_ADDR_DUMMY        0xFF

#define COMM_ADDR_NAME0         ADDR_NAME0
#define COMM_ADDR_NAME1         ADDR_NAME1
#define COMM_ADDR_VERSION       ADDR_VERSION

// current name and version values
#define NOVENA_BOARD_NAME0      "PVT1"
#define NOVENA_BOARD_NAME1      "    "
#define NOVENA_BOARD_VERSION    "0.10"

#define EIM_INTERFACE_NAME0     "eim "
#define EIM_INTERFACE_NAME1     "    "
#define EIM_INTERFACE_VERSION   "0.10"

#define I2C_INTERFACE_NAME0     "i2c "
#define I2C_INTERFACE_NAME1     "    "
#define I2C_INTERFACE_VERSION   "0.10"


//------------------------------------------------------------------
// Hash cores
//------------------------------------------------------------------
// addresses common to all hash cores
#define ADDR_BLOCK              0x10
#define ADDR_DIGEST             0x20      // except SHA512

// SHA-1 core
#define SHA1_ADDR_NAME0         ADDR_NAME0
#define SHA1_ADDR_NAME1         ADDR_NAME1
#define SHA1_ADDR_VERSION       ADDR_VERSION
#define SHA1_ADDR_CTRL          ADDR_CTRL
#define SHA1_ADDR_STATUS        ADDR_STATUS
#define SHA1_ADDR_BLOCK         ADDR_BLOCK
#define SHA1_ADDR_DIGEST        ADDR_DIGEST
#define SHA1_BLOCK_LEN          bitsToBytes(512)
#define SHA1_LENGTH_LEN         bitsToBytes(64)
#define SHA1_DIGEST_LEN         bitsToBytes(160)

#define SHA256_ADDR_NAME0       ADDR_NAME0
#define SHA256_ADDR_NAME1       ADDR_NAME1
#define SHA256_ADDR_VERSION     ADDR_VERSION
#define SHA256_ADDR_CTRL        ADDR_CTRL
#define SHA256_ADDR_STATUS      ADDR_STATUS
#define SHA256_ADDR_BLOCK       ADDR_BLOCK
#define SHA256_ADDR_DIGEST      ADDR_DIGEST
#define SHA256_BLOCK_LEN        bitsToBytes(512)
#define SHA256_LENGTH_LEN       bitsToBytes(64)
#define SHA256_DIGEST_LEN       bitsToBytes(256)

#define SHA512_ADDR_NAME0       ADDR_NAME0
#define SHA512_ADDR_NAME1       ADDR_NAME1
#define SHA512_ADDR_VERSION     ADDR_VERSION
#define SHA512_ADDR_CTRL        ADDR_CTRL
#define SHA512_ADDR_STATUS      ADDR_STATUS
#define SHA512_ADDR_BLOCK       ADDR_BLOCK
#define SHA512_ADDR_DIGEST      0x40
#define SHA512_BLOCK_LEN        bitsToBytes(1024)
#define SHA512_LENGTH_LEN       bitsToBytes(128)
#define SHA512_224_DIGEST_LEN   bitsToBytes(224)
#define SHA512_256_DIGEST_LEN   bitsToBytes(256)
#define SHA384_DIGEST_LEN       bitsToBytes(384)
#define SHA512_DIGEST_LEN       bitsToBytes(512)
#define MODE_SHA_512_224        0 << 2
#define MODE_SHA_512_256        1 << 2
#define MODE_SHA_384            2 << 2
#define MODE_SHA_512            3 << 2

// current name and version values
#define SHA1_NAME0              "sha1"
#define SHA1_NAME1              "    "
#define SHA1_VERSION            "0.50"

#define SHA256_NAME0            "sha2"
#define SHA256_NAME1            "-256"
#define SHA256_VERSION          "0.81"

#define SHA512_NAME0            "sha2"
#define SHA512_NAME1            "-512"
#define SHA512_VERSION          "0.80"


//-----------------------------------------------------------------
// TRNG cores
//-----------------------------------------------------------------
// addresses and codes for the TRNG cores */
#define TRNG_ADDR_NAME0         ADDR_NAME0
#define TRNG_ADDR_NAME1         ADDR_NAME1
#define TRNG_ADDR_VERSION       ADDR_VERSION
#define TRNG_ADDR_CTRL          0x10
#define TRNG_CTRL_DISCARD       1
#define TRNG_CTRL_TEST_MODE     2
#define TRNG_ADDR_STATUS        0x11
// no status bits defined (yet)
#define TRNG_ADDR_DELAY         0x13

#define ENTROPY1_ADDR_NAME0     ADDR_NAME0
#define ENTROPY1_ADDR_NAME1     ADDR_NAME1
#define ENTROPY1_ADDR_VERSION   ADDR_VERSION
#define ENTROPY1_ADDR_CTRL      0x10
#define ENTROPY1_CTRL_ENABLE    1
#define ENTROPY1_ADDR_STATUS    0x11
#define ENTROPY1_STATUS_VALID   1
#define ENTROPY1_ADDR_ENTROPY   0x20
#define ENTROPY1_ADDR_DELTA     0x30

#define ENTROPY2_ADDR_NAME0     ADDR_NAME0
#define ENTROPY2_ADDR_NAME1     ADDR_NAME1
#define ENTROPY2_ADDR_VERSION   ADDR_VERSION
#define ENTROPY2_ADDR_CTRL      0x10
#define ENTROPY2_CTRL_ENABLE    1
#define ENTROPY2_ADDR_STATUS    0x11
#define ENTROPY2_STATUS_VALID   1
#define ENTROPY2_ADDR_OPA       0x18
#define ENTROPY2_ADDR_OPB       0x19
#define ENTROPY2_ADDR_ENTROPY   0x20
#define ENTROPY2_ADDR_RAW       0x21
#define ENTROPY2_ADDR_ROSC      0x22

#define MIXER_ADDR_NAME0        ADDR_NAME0
#define MIXER_ADDR_NAME1        ADDR_NAME1
#define MIXER_ADDR_VERSION      ADDR_VERSION
#define MIXER_ADDR_CTRL         0x10
#define MIXER_CTRL_ENABLE       1
#define MIXER_CTRL_RESTART      2
#define MIXER_ADDR_STATUS       0x11
// no status bits defined (yet)
#define MIXER_ADDR_TIMEOUT      0x20

#define CSPRNG_ADDR_NAME0       ADDR_NAME0
#define CSPRNG_ADDR_NAME1       ADDR_NAME1
#define CSPRNG_ADDR_VERSION     ADDR_VERSION
#define CSPRNG_ADDR_CTRL        0x10
#define CSPRNG_CTRL_ENABLE      1
#define CSPRNG_CTRL_SEED        2
#define CSPRNG_ADDR_STATUS      0x11
#define CSPRNG_STATUS_VALID     1
#define CSPRNG_ADDR_RANDOM      0x20
#define CSPRNG_ADDR_NROUNDS     0x40
#define CSPRNG_ADDR_NBLOCKS_LO  0x41
#define CSPRNG_ADDR_NBLOCKS_HI  0x42

// current name and version values
#define TRNG_NAME0              "trng"
#define TRNG_NAME1              "    "
#define TRNG_VERSION            "0.51"

#define AVALANCHE_ENTROPY_NAME0   "extn"
#define AVALANCHE_ENTROPY_NAME1   "oise"
#define AVALANCHE_ENTROPY_VERSION "0.10"

#define ROSC_ENTROPY_NAME0      "rosc"
#define ROSC_ENTROPY_NAME1      " ent"
#define ROSC_ENTROPY_VERSION    "0.10"

#define MIXER_NAME0             "rngm"
#define MIXER_NAME1             "ixer"
#define MIXER_VERSION           "0.50"

#define CSPRNG_NAME0            "cspr"
#define CSPRNG_NAME1            "ng  "
#define CSPRNG_VERSION          "0.50"


// -----------------------------------------------------------------
// Cipher cores
// -----------------------------------------------------------------
// AES core
#define AES_ADDR_NAME0          ADDR_NAME0
#define AES_ADDR_NAME1          ADDR_NAME1
#define AES_ADDR_VERSION        ADDR_VERSION
#define AES_ADDR_CTRL           ADDR_CTRL
#define AES_ADDR_STATUS         ADDR_STATUS

#define AES_ADDR_CONFIG         0x0a
#define AES_CONFIG_ENCDEC       1
#define AES_CONFIG_KEYLEN       2

#define AES_ADDR_KEY0           0x10
#define AES_ADDR_KEY1           0x11
#define AES_ADDR_KEY2           0x12
#define AES_ADDR_KEY3           0x13
#define AES_ADDR_KEY4           0x14
#define AES_ADDR_KEY5           0x15
#define AES_ADDR_KEY6           0x16
#define AES_ADDR_KEY7           0x17

#define AES_ADDR_BLOCK0         0x20
#define AES_ADDR_BLOCK1         0x21
#define AES_ADDR_BLOCK2         0x22
#define AES_ADDR_BLOCK3         0x23

#define AES_ADDR_RESULT0        0x30
#define AES_ADDR_RESULT1        0x31
#define AES_ADDR_RESULT2        0x32
#define AES_ADDR_RESULT3        0x33

// current name and version values
#define AES_CORE_NAME0          "aes "
#define AES_CORE_NAME1          "    "
#define AES_CORE_VERSION        "0.80"


// Chacha core
#define CHACHA_ADDR_NAME0       ADDR_NAME0
#define CHACHA_ADDR_NAME1       ADDR_NAME1
#define CHACHA_ADDR_VERSION     ADDR_VERSION
#define CHACHA_ADDR_CTRL        ADDR_CTRL
#define CHACHA_ADDR_STATUS      ADDR_STATUS

#define CHACHA_ADDR_KEYLEN      0x0a
#define CHACHA_KEYLEN           1

#define CHACHA_ADDR_ROUNDS      0x0b

#define CHACHA_ADDR_KEY0        0x10
#define CHACHA_ADDR_KEY1        0x11
#define CHACHA_ADDR_KEY2        0x12
#define CHACHA_ADDR_KEY3        0x13
#define CHACHA_ADDR_KEY4        0x14
#define CHACHA_ADDR_KEY5        0x15
#define CHACHA_ADDR_KEY6        0x16
#define CHACHA_ADDR_KEY7        0x17

#define CHACHA_ADDR_IV0         0x20
#define CHACHA_ADDR_IV1         0x21

#define CHACHA_ADDR_DATA_IN0    0x40
#define CHACHA_ADDR_DATA_IN1    0x41
#define CHACHA_ADDR_DATA_IN2    0x42
#define CHACHA_ADDR_DATA_IN3    0x43
#define CHACHA_ADDR_DATA_IN4    0x44
#define CHACHA_ADDR_DATA_IN5    0x45
#define CHACHA_ADDR_DATA_IN6    0x46
#define CHACHA_ADDR_DATA_IN7    0x47
#define CHACHA_ADDR_DATA_IN8    0x48
#define CHACHA_ADDR_DATA_IN9    0x49
#define CHACHA_ADDR_DATA_IN10   0x4a
#define CHACHA_ADDR_DATA_IN11   0x4b
#define CHACHA_ADDR_DATA_IN12   0x4c
#define CHACHA_ADDR_DATA_IN13   0x4d
#define CHACHA_ADDR_DATA_IN14   0x4e
#define CHACHA_ADDR_DATA_IN15   0x4f

#define CHACHA_ADDR_DATA_OUT0   0x80
#define CHACHA_ADDR_DATA_OUT1   0x81
#define CHACHA_ADDR_DATA_OUT2   0x82
#define CHACHA_ADDR_DATA_OUT3   0x83
#define CHACHA_ADDR_DATA_OUT4   0x84
#define CHACHA_ADDR_DATA_OUT5   0x85
#define CHACHA_ADDR_DATA_OUT6   0x86
#define CHACHA_ADDR_DATA_OUT7   0x87
#define CHACHA_ADDR_DATA_OUT8   0x88
#define CHACHA_ADDR_DATA_OUT9   0x89
#define CHACHA_ADDR_DATA_OUT10  0x8a
#define CHACHA_ADDR_DATA_OUT11  0x8b
#define CHACHA_ADDR_DATA_OUT12  0x8c
#define CHACHA_ADDR_DATA_OUT13  0x8d
#define CHACHA_ADDR_DATA_OUT14  0x8e
#define CHACHA_ADDR_DATA_OUT15  0x8f

// current name and version values
#define CHACHA_NAME0            "chac"
#define CHACHA_NAME1            "ha  "
#define CHACHA_VERSION          "0.80"


// -----------------------------------------------------------------
// Math cores
// -----------------------------------------------------------------
// Modular exponentiation core
#define MODEXP_ADDR_NAME0       ADDR_NAME0
#define MODEXP_ADDR_NAME1       ADDR_NAME1
#define MODEXP_ADDR_VERSION     ADDR_VERSION
#define MODEXP_ADDR_CTRL        ADDR_CTRL
#define MODEXP_CTRL_INIT_BIT    1
#define MODEXP_CTRL_NEXT_BIT    2
#define MODEXP_ADDR_STATUS      ADDR_STATUS

#define MODEXP_ADDR_DELAY       0x13
#define MODEXP_STATUS_READY     1

#define MODEXP_MODULUS_LENGTH   0x20
#define MODEXP_EXPONENT_LENGTH  0x21
#define MODEXP_LENGTH           0x22

#define MODEXP_MODULUS_PTR_RST  0x30
#define MODEXP_MODULUS_DATA     0x31

#define MODEXP_EXPONENT_PTR_RST 0x40
#define MODEXP_EXPONENT_DATA    0x41

#define MODEXP_MESSAGE_PTR_RST  0x50
#define MODEXP_MESSAGE_DATA     0x51

#define MODEXP_RESULT_PTR_RST   0x60
#define MODEXP_RESULT_DATA      0x61

#define MODEXP_NAME0            "mode"
#define MODEXP_NAME1            "xp  "
#define MODEXP_VERSION          "0.51"

// Experimental ModexpS6 core.
// XXX AT THE SAME CORE PREFIX - YOU CAN'T HAVE BOTH AT THE SAME TIME
// Well, under the old scheme, anyway, remains to be seen with the new scheme
#define MODEXPS6_ADDR_NAME0          ADDR_NAME0
#define MODEXPS6_ADDR_NAME1          ADDR_NAME1
#define MODEXPS6_ADDR_VERSION        ADDR_VERSION
#define MODEXPS6_ADDR_CTRL           ADDR_CTRL
#define MODEXPS6_CTRL_INIT_BIT       1
#define MODEXPS6_CTRL_NEXT_BIT       2
#define MODEXPS6_ADDR_STATUS         ADDR_STATUS

/* 4096-bit operands are stored as 128 words of 32 bits */
#define MODEXPS6_OPERAND_SIZE        4096/32

#define MODEXPS6_ADDR_REGISTERS      0 * MODEXPS6_OPERAND_SIZE
#define MODEXPS6_ADDR_OPERANDS       4 * MODEXPS6_OPERAND_SIZE

#define MODEXPS6_ADDR_MODE           MODEXPS6_ADDR_REGISTERS + 0x10
#define MODEXPS6_ADDR_MODULUS_WIDTH  MODEXPS6_ADDR_REGISTERS + 0x11
#define MODEXPS6_ADDR_EXPONENT_WIDTH MODEXPS6_ADDR_REGISTERS + 0x12

/* addresses of block memories for operands */
#define MODEXPS6_ADDR_MODULUS        MODEXPS6_ADDR_OPERANDS + 0 * MODEXPS6_OPERAND_SIZE
#define MODEXPS6_ADDR_MESSAGE        MODEXPS6_ADDR_OPERANDS + 1 * MODEXPS6_OPERAND_SIZE
#define MODEXPS6_ADDR_EXPONENT       MODEXPS6_ADDR_OPERANDS + 2 * MODEXPS6_OPERAND_SIZE
#define MODEXPS6_ADDR_RESULT         MODEXPS6_ADDR_OPERANDS + 3 * MODEXPS6_OPERAND_SIZE

#define MODEXPS6_NAME0            "mode"
#define MODEXPS6_NAME1            "xps6"
#define MODEXPS6_VERSION          "0.10"


//------------------------------------------------------------------
// Test case public functions
//------------------------------------------------------------------
struct core_info {
    char name[8];
    char version[4];
    off_t base;
    struct core_info *next;
};
struct core_info *tc_core_first(char *name);
struct core_info *tc_core_next(struct core_info *node, char *name);
off_t tc_core_base(char *name);

void tc_set_debug(int onoff);
int tc_write(off_t offset, const uint8_t *buf, size_t len);
int tc_read(off_t offset, uint8_t *buf, size_t len);
int tc_expected(off_t offset, const uint8_t *expected, size_t len);
int tc_init(off_t offset);
int tc_next(off_t offset);
int tc_wait(off_t offset, uint8_t status, int *count);
int tc_wait_ready(off_t offset);
int tc_wait_valid(off_t offset);


//------------------------------------------------------------------
// I2C configuration
// Only used in I2C, but not harmful to define for EIM
//------------------------------------------------------------------
#define I2C_dev                 "/dev/i2c-2"
#define I2C_addr                0x0f
#define I2C_SLAVE               0x0703


//======================================================================
// EOF cryptech.h
//======================================================================
