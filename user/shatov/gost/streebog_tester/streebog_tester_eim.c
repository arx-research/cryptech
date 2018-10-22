#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <signal.h>

#include "novena-eim.h"

int debug = 0;
int quiet = 0;
int repeat = 0;

#define SEGMENT_OFFSET_GLOBALS  EIM_BASE_ADDR + 0x000000
#define SEGMENT_OFFSET_HASHES   EIM_BASE_ADDR + 0x010000
#define SEGMENT_OFFSET_RNGS     EIM_BASE_ADDR + 0x020000
#define SEGMENT_OFFSET_CIPHERS  EIM_BASE_ADDR + 0x030000

/* addresses and codes common to all cores */
#define ADDR_NAME0              (0x00 << 2)
#define ADDR_NAME1              (0x01 << 2)
#define ADDR_VERSION            (0x02 << 2)

/* addresses and codes common to all hash cores */
#define ADDR_CTRL               (0x08 << 2)
#define CTRL_INIT_CMD           1
#define CTRL_NEXT_CMD           2
#define ADDR_STATUS             (0x09 << 2)
#define STATUS_READY_BIT        1
#define STATUS_VALID_BIT        2
#define ADDR_BLOCK              (0x10 << 2)
#define ADDR_DIGEST             (0x20 << 2)
#define HASH_CORE_SIZE          (0x100 << 2)

/* addresses and codes for the specific hash cores */
#define STREEBOG_ADDR_BASE        SEGMENT_OFFSET_HASHES + (3*HASH_CORE_SIZE)
#define STREEBOG_ADDR_NAME0       STREEBOG_ADDR_BASE + ADDR_NAME0
#define STREEBOG_ADDR_NAME1       STREEBOG_ADDR_BASE + ADDR_NAME1
#define STREEBOG_ADDR_VERSION     STREEBOG_ADDR_BASE + ADDR_VERSION
#define STREEBOG_ADDR_CTRL        STREEBOG_ADDR_BASE + ADDR_CTRL
#define STREEBOG_ADDR_STATUS      STREEBOG_ADDR_BASE + ADDR_STATUS
#define STREEBOG_ADDR_BLOCK_BITS  STREEBOG_ADDR_BASE + (0x0a << 2)
#define STREEBOG_ADDR_MODE		  STREEBOG_ADDR_BASE + (0x0b << 2)
#define STREEBOG_ADDR_BLOCK       STREEBOG_ADDR_BASE + ADDR_BLOCK
#define STREEBOG_ADDR_DIGEST      STREEBOG_ADDR_BASE + ADDR_DIGEST
#define CTRL_FINAL_CMD            4
#define STREEBOG_MODE_512         0
#define STREEBOG_MODE_256         1
#define STREEBOG_BLOCK_LEN        512 / 8
#define STREEBOG_DIGEST_LEN_512   512 / 8
#define STREEBOG_DIGEST_LEN_256   256 / 8


const uint8_t GOST_SINGLE[] =
{ 0x01, 0x32, 0x31, 0x30, 0x39, 0x38, 0x37, 0x36,
  0x35, 0x34, 0x33, 0x32, 0x31, 0x30, 0x39, 0x38,
  0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30,
  0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32,
  0x31, 0x30, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34,
  0x33, 0x32, 0x31, 0x30, 0x39, 0x38, 0x37, 0x36,
  0x35, 0x34, 0x33, 0x32, 0x31, 0x30, 0x39, 0x38,
  0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30 };

const uint8_t GOST_DOUBLE_FIRST[] = 
{ 0xfb, 0xea, 0xfa, 0xeb, 0xef, 0x20, 0xff, 0xfb,
  0xf0, 0xe1, 0xe0, 0xf0, 0xf5, 0x20, 0xe0, 0xed,
  0x20, 0xe8, 0xec, 0xe0, 0xeb, 0xe5, 0xf0, 0xf2,
  0xf1, 0x20, 0xff, 0xf0, 0xee, 0xec, 0x20, 0xf1,
  0x20, 0xfa, 0xf2, 0xfe, 0xe5, 0xe2, 0x20, 0x2c,
  0xe8, 0xf6, 0xf3, 0xed, 0xe2, 0x20, 0xe8, 0xe6,
  0xee, 0xe1, 0xe8, 0xf0, 0xf2, 0xd1, 0x20, 0x2c,
  0xe8, 0xf0, 0xf2, 0xe5, 0xe2, 0x20, 0xe5, 0xd1 };
  
const uint8_t GOST_DOUBLE_SECOND[] = 
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  0xfb, 0xe2, 0xe5, 0xf0, 0xee, 0xe3, 0xc8, 0x20 };
  
const uint32_t GOST_SINGLE_LENGTH = 504;

const uint32_t GOST_DOUBLE_LENGTH_FIRST  = 512;
const uint32_t GOST_DOUBLE_LENGTH_SECOND =  64;

const uint8_t GOST_SINGLE_DIGEST_512[] =
{ 0x48, 0x6f, 0x64, 0xc1, 0x91, 0x78, 0x79, 0x41,
  0x7f, 0xef, 0x08, 0x2b, 0x33, 0x81, 0xa4, 0xe2,
  0x11, 0xc3, 0x24, 0xf0, 0x74, 0x65, 0x4c, 0x38,
  0x82, 0x3a, 0x7b, 0x76, 0xf8, 0x30, 0xad, 0x00,
  0xfa, 0x1f, 0xba, 0xe4, 0x2b, 0x12, 0x85, 0xc0,
  0x35, 0x2f, 0x22, 0x75, 0x24, 0xbc, 0x9a, 0xb1,
  0x62, 0x54, 0x28, 0x8d, 0xd6, 0x86, 0x3d, 0xcc,
  0xd5, 0xb9, 0xf5, 0x4a, 0x1a, 0xd0, 0x54, 0x1b };
  
const uint8_t GOST_SINGLE_DIGEST_256[] =
{ 0x00, 0x55, 0x7b, 0xe5, 0xe5, 0x84, 0xfd, 0x52,
  0xa4, 0x49, 0xb1, 0x6b, 0x02, 0x51, 0xd0, 0x5d,
  0x27, 0xf9, 0x4a, 0xb7, 0x6c, 0xba, 0xa6, 0xda,
  0x89, 0x0b, 0x59, 0xd8, 0xef, 0x1e, 0x15, 0x9d };
  
const uint8_t GOST_DOUBLE_DIGEST_512[] =
{ 0x28, 0xfb, 0xc9, 0xba, 0xda, 0x03, 0x3b, 0x14,
  0x60, 0x64, 0x2b, 0xdc, 0xdd, 0xb9, 0x0c, 0x3f,
  0xb3, 0xe5, 0x6c, 0x49, 0x7c, 0xcd, 0x0f, 0x62,
  0xb8, 0xa2, 0xad, 0x49, 0x35, 0xe8, 0x5f, 0x03,
  0x76, 0x13, 0x96, 0x6d, 0xe4, 0xee, 0x00, 0x53,
  0x1a, 0xe6, 0x0f, 0x3b, 0x5a, 0x47, 0xf8, 0xda,
  0xe0, 0x69, 0x15, 0xd5, 0xf2, 0xf1, 0x94, 0x99,
  0x6f, 0xca, 0xbf, 0x26, 0x22, 0xe6, 0x88, 0x1e };
  
const uint8_t GOST_DOUBLE_DIGEST_256[] =
{ 0x50, 0x8f, 0x7e, 0x55, 0x3c, 0x06, 0x50, 0x1d,
  0x74, 0x9a, 0x66, 0xfc, 0x28, 0xc6, 0xca, 0xc0,
  0xb0, 0x05, 0x74, 0x6d, 0x97, 0x53, 0x7f, 0xa8,
  0x5d, 0x9e, 0x40, 0x90, 0x4e, 0xfe, 0xd2, 0x9d };


/* ---------------- test-case low-level code ---------------- */

void dump(char *label, const uint8_t *buf, int len)
{
    if (debug) {
        int i;
        printf("%s [", label);
        for (i = 0; i < len; ++i)
            printf(" %02x", buf[i]);
        printf(" ]\n");
    }
}

int tc_write(off_t offset, const uint8_t *buf, int len)
{
    dump("write ", buf, len);

    for (; len > 0; offset += 4, buf += 4, len -= 4) {
        uint32_t val;
        val = htonl(*(uint32_t *)buf);
        eim_write_32(offset, &val);
    }

    return 0;
}

int tc_read(off_t offset, uint8_t *buf, int len)
{
    uint8_t *rbuf = buf;
    int rlen = len;

    for (; rlen > 0; offset += 4, rbuf += 4, rlen -= 4) {
        uint32_t val;
        eim_read_32(offset, &val);
        *(uint32_t *)rbuf = ntohl(val);
    }

    dump("read  ", buf, len);

    return 0;
}

int tc_expected(off_t offset, const uint8_t *expected, int len)
{
    uint8_t *buf;
    int i;

    buf = malloc(len);
    if (buf == NULL) {
        perror("malloc");
        return 1;
    }
    dump("expect", expected, len);

    if (tc_read(offset, buf, len) != 0)
        goto errout;

    for (i = 0; i < len; ++i)
        if (buf[i] != expected[i]) {
            fprintf(stderr, "response byte %d: expected 0x%02x, got 0x%02x\n",
                    i, expected[i], buf[i]);
            goto errout;
        }

    free(buf);
    return 0;
errout:
    free(buf);
    return 1;
}

int tc_block_bits(off_t offset, uint32_t bits)
{
    uint8_t buf[4] = {0, 0, (uint8_t)(bits >> 8), (uint8_t)bits};
    return tc_write(offset, buf, 4);
}

int tc_mode(off_t offset, char mode)
{
    uint8_t buf[4] = { 0, 0, 0, mode };

    return tc_write(offset, buf, 4);
}

int tc_init(off_t offset)
{
    uint8_t buf[4] = { 0, 0, 0, CTRL_INIT_CMD };

    return tc_write(offset, buf, 4);
}

int tc_next(off_t offset)
{
	uint8_t buf_clear[4] = { 0, 0, 0, 0 };
	uint8_t buf_next[4]  = { 0, 0, 0, CTRL_NEXT_CMD };
	
    tc_write(offset, buf_clear, 4);
    
    return tc_write(offset, buf_next, 4);
}

int tc_final(off_t offset)
{
    uint8_t buf[4] = { 0, 0, 0, CTRL_FINAL_CMD };

    return tc_write(offset, buf, 4);
}

int tc_wait(off_t offset, uint8_t status)
{
    uint8_t buf[4];

#if 0
    do {
        if (tc_read(offset, buf, 4) != 0)
            return 1;
    } while (!(buf[3] & status));

    return 0;
#else
    int i;
    for (i = 0; i < 100; ++i) {
        if (tc_read(offset, buf, 4) != 0)
            return 1;
        if (buf[3] & status)
            return 0;
    }
    fprintf(stderr, "tc_wait timed out\n");
    return 1;
#endif
}

int tc_wait_ready(off_t offset)
{
    return tc_wait(offset, STATUS_READY_BIT);
}

int tc_wait_valid(off_t offset)
{
    return tc_wait(offset, STATUS_VALID_BIT);
}


/* ---------------- Streebog test cases ---------------- */

/* TC0: Read name and version from Streebog core. */
int TC0(void)
{
    uint8_t name0[4]   = { 's', 't', 'r', 'e'};
    uint8_t name1[4]   = { 'e', 'b', 'o', 'g'};
    uint8_t version[4] = { '0', '.', '1', '0'};

    if (!quiet)
        printf("TC0: Reading name and version words from Streebog core.\n");

    return
        tc_expected(STREEBOG_ADDR_NAME0, name0, 4) ||
        tc_expected(STREEBOG_ADDR_NAME1, name1, 4) ||
        tc_expected(STREEBOG_ADDR_VERSION, version, 4);
}

/* TC1: Streebog single block message for 512-bit hash mode. */
int TC1(void)
{
    const uint8_t *block = GOST_SINGLE;
    const uint8_t *expected = GOST_SINGLE_DIGEST_512;
    int ret;

    if (!quiet)
        printf("TC1: Short (single block) message test for Streebog (512-bit mode).\n");

	/* Enable 512-bit mode. */
	tc_mode(STREEBOG_ADDR_MODE, STREEBOG_MODE_512);
	/* Start initial block hashing. */
    tc_init(STREEBOG_ADDR_CTRL);
	
    /* Write block to Streebog. */
    tc_write(STREEBOG_ADDR_BLOCK, block, STREEBOG_BLOCK_LEN);
	/* Write block length to Streebog. */
	tc_block_bits(STREEBOG_ADDR_BLOCK_BITS, GOST_SINGLE_LENGTH);
	/* Process block. */
	tc_next(STREEBOG_ADDR_CTRL);
	/* Wait for block to be processed. */
	tc_wait_ready(STREEBOG_ADDR_STATUS);

	/* Perform final transformation. */
	tc_final(STREEBOG_ADDR_CTRL);
	/* Wait for hash to be produces. */
    tc_wait_valid(STREEBOG_ADDR_STATUS);
    /* Extract the digest. */
    ret = tc_expected(STREEBOG_ADDR_DIGEST, expected, STREEBOG_DIGEST_LEN_512);
	
	/* Make sure that core is idle. */
	tc_wait_ready(STREEBOG_ADDR_STATUS);
	
    return ret;
}

/* TC2: Streebog double block message for 512-bit hash mode. */
int TC2(void)
{
    const uint8_t *block1 = GOST_DOUBLE_FIRST;
	const uint8_t *block2 = GOST_DOUBLE_SECOND;
    const uint8_t *expected = GOST_DOUBLE_DIGEST_512;
    int ret;

    if (!quiet)
        printf("TC2: Long (double block) message test for Streebog (512-bit mode).\n");

	/* Enable 512-bit mode. */
	tc_mode(STREEBOG_ADDR_MODE, STREEBOG_MODE_512);
	/* Start initial block hashing. */
    tc_init(STREEBOG_ADDR_CTRL);
	
    /* Write block to Streebog. */
    tc_write(STREEBOG_ADDR_BLOCK, block1, STREEBOG_BLOCK_LEN);
	/* Write block length to Streebog. */
	tc_block_bits(STREEBOG_ADDR_BLOCK_BITS, GOST_DOUBLE_LENGTH_FIRST);
	/* Process block. */
	tc_next(STREEBOG_ADDR_CTRL);
	/* Wait for block to be processed. */
	tc_wait_ready(STREEBOG_ADDR_STATUS);
	
	/* Write block to Streebog. */
    tc_write(STREEBOG_ADDR_BLOCK, block2, STREEBOG_BLOCK_LEN);
	/* Write block length to Streebog. */
	tc_block_bits(STREEBOG_ADDR_BLOCK_BITS, GOST_DOUBLE_LENGTH_SECOND);
	/* Process block. */
	tc_next(STREEBOG_ADDR_CTRL);
	/* Wait for block to be processed. */
	tc_wait_ready(STREEBOG_ADDR_STATUS);

	/* Perform final transformation. */
	tc_final(STREEBOG_ADDR_CTRL);
	/* Wait for hash to be produces. */
    tc_wait_valid(STREEBOG_ADDR_STATUS);
    /* Extract the digest. */
    ret = tc_expected(STREEBOG_ADDR_DIGEST, expected, STREEBOG_DIGEST_LEN_512);
	
	/* Make sure that core is idle. */
	tc_wait_ready(STREEBOG_ADDR_STATUS);
	
    return ret;
}


/* TC3: Streebog single block message for 256-bit hash mode. */
int TC3(void)
{
    const uint8_t *block = GOST_SINGLE;
    const uint8_t *expected = GOST_SINGLE_DIGEST_256;
    int ret;

    if (!quiet)
        printf("TC3: Short (single block) message test for Streebog (256-bit mode).\n");

	/* Enable 512-bit mode. */
	tc_mode(STREEBOG_ADDR_MODE, STREEBOG_MODE_256);
	/* Start initial block hashing. */
    tc_init(STREEBOG_ADDR_CTRL);
	
    /* Write block to Streebog. */
    tc_write(STREEBOG_ADDR_BLOCK, block, STREEBOG_BLOCK_LEN);
	/* Write block length to Streebog. */
	tc_block_bits(STREEBOG_ADDR_BLOCK_BITS, GOST_SINGLE_LENGTH);
	/* Process block. */
	tc_next(STREEBOG_ADDR_CTRL);
	/* Wait for block to be processed. */
	tc_wait_ready(STREEBOG_ADDR_STATUS);

	/* Perform final transformation. */
	tc_final(STREEBOG_ADDR_CTRL);
	/* Wait for hash to be produces. */
    tc_wait_valid(STREEBOG_ADDR_STATUS);
    /* Extract the digest. */
    ret = tc_expected(STREEBOG_ADDR_DIGEST, expected, STREEBOG_DIGEST_LEN_256);
	
	/* Make sure that core is idle. */
	tc_wait_ready(STREEBOG_ADDR_STATUS);
	
    return ret;
}


/* TC4: Streebog double block message for 256-bit hash mode. */
int TC4(void)
{
	const uint8_t *block1 = GOST_DOUBLE_FIRST;
	const uint8_t *block2 = GOST_DOUBLE_SECOND;
    const uint8_t *expected = GOST_DOUBLE_DIGEST_256;
    int ret;

    if (!quiet)
        printf("TC4: Long (double block) message test for Streebog (256-bit mode).\n");

	/* Enable 512-bit mode. */
	tc_mode(STREEBOG_ADDR_MODE, STREEBOG_MODE_256);
	/* Start initial block hashing. */
    tc_init(STREEBOG_ADDR_CTRL);
	
    /* Write block to Streebog. */
    tc_write(STREEBOG_ADDR_BLOCK, block1, STREEBOG_BLOCK_LEN);
	/* Write block length to Streebog. */
	tc_block_bits(STREEBOG_ADDR_BLOCK_BITS, GOST_DOUBLE_LENGTH_FIRST);
	/* Process block. */
	tc_next(STREEBOG_ADDR_CTRL);
	/* Wait for block to be processed. */
	tc_wait_ready(STREEBOG_ADDR_STATUS);
	
	/* Write block to Streebog. */
    tc_write(STREEBOG_ADDR_BLOCK, block2, STREEBOG_BLOCK_LEN);
	/* Write block length to Streebog. */
	tc_block_bits(STREEBOG_ADDR_BLOCK_BITS, GOST_DOUBLE_LENGTH_SECOND);
	/* Process block. */
	tc_next(STREEBOG_ADDR_CTRL);
	/* Wait for block to be processed. */
	tc_wait_ready(STREEBOG_ADDR_STATUS);

	/* Perform final transformation. */
	tc_final(STREEBOG_ADDR_CTRL);
	/* Wait for hash to be produces. */
    tc_wait_valid(STREEBOG_ADDR_STATUS);
    /* Extract the digest. */
    ret = tc_expected(STREEBOG_ADDR_DIGEST, expected, STREEBOG_DIGEST_LEN_256);
	
	/* Make sure that core is idle. */
	tc_wait_ready(STREEBOG_ADDR_STATUS);
	
    return ret;
}

/* ---------------- main ---------------- */

/* signal handler for ctrl-c to end repeat testing */
unsigned long iter = 0;
struct timeval tv_start, tv_end;
void sighandler(int unused)
{
    double tv_diff;

    gettimeofday(&tv_end, NULL);
    tv_diff = (double)(tv_end.tv_sec - tv_start.tv_sec) +
        (double)(tv_end.tv_usec - tv_start.tv_usec)/1000000;
    printf("\n%lu iterations in %.3f seconds (%.3f iterations/sec)\n",
           iter, tv_diff, (double)iter/tv_diff);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    typedef int (*tcfp)(void);
    tcfp all_tests[] = { TC0, TC1, TC2, TC3, TC4 };

    char *usage = "Usage: %s [-h] [-d] [-q] [-r] tc...\n";
    int i, j, opt;

    while ((opt = getopt(argc, argv, "h?dqr")) != -1) {
        switch (opt) {
        case 'h':
        case '?':
            printf(usage, argv[0]);
            return EXIT_SUCCESS;
        case 'd':
            debug = 1;
            break;
        case 'q':
            quiet = 1;
            break;
        case 'r':
            repeat = 1;
            break;
        default:
            fprintf(stderr, usage, argv[0]);
            return EXIT_FAILURE;
        }
    }

    /* set up EIM */
    if (eim_setup() != 0) {
        fprintf(stderr, "EIM setup failed\n");
        return EXIT_FAILURE;
    }

    /* repeat one test until interrupted */
    if (repeat) {
        tcfp tc;
        if (optind != argc - 1) {
            fprintf(stderr, "only one test case can be repeated\n");
            return EXIT_FAILURE;
        }
        j = atoi(argv[optind]);
        if (j < 0 || j >= sizeof(all_tests)/sizeof(all_tests[0])) {
            fprintf(stderr, "invalid test number %s\n", argv[optind]);
            return EXIT_FAILURE;
        }
        tc = (all_tests[j]);
        srand(time(NULL));
        signal(SIGINT, sighandler);
        gettimeofday(&tv_start, NULL);
        while (1) {
            ++iter;
            if ((iter & 0xffff) == 0) {
                printf(".");
                fflush(stdout);
            }
            if (tc() != 0)
                sighandler(0);
        }
        return EXIT_SUCCESS;    /*NOTREACHED*/
    }

    /* no args == run all tests */
    if (optind >= argc) {
        for (j = 0; j < sizeof(all_tests)/sizeof(all_tests[0]); ++j)
            if (all_tests[j]() != 0)
                return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }

    /* run one or more tests (by number) or groups of tests (by name) */
    for (i = optind; i < argc; ++i) {
        if (strcmp(argv[i], "all") == 0) {
            for (j = 0; j < sizeof(all_tests)/sizeof(all_tests[0]); ++j)
                if (all_tests[j]() != 0)
                    return EXIT_FAILURE;
        }
        else if (isdigit(argv[i][0]) &&
                 (((j = atoi(argv[i])) >= 0) &&
                  (j < sizeof(all_tests)/sizeof(all_tests[0])))) {
            if (all_tests[j]() != 0)
                return EXIT_FAILURE;
        }
        else {
            fprintf(stderr, "unknown test case %s\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
