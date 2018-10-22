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
#include <assert.h>

#include "cryptech.h"
#include "test-rsa.h"
#include "test-modexp-for-pavel.h"

int quiet = 0;
int repeat = 0;

int tc_width(off_t offset, uint32_t length)
{
  length = htonl(length);	// !

  uint8_t width[4];
  memcpy(width, &length, 4);

  return tc_write(offset, width, sizeof(width));
}

/*
 * Utility to madly swap 32-bit words within an operand so that the
 * first word becomes the last word and so forth.
 */

static void two_card_monty(void *output, const void * const input, const size_t byte_count)
{
  const size_t word_count = byte_count / 4;
  const uint32_t * const i32 = input;
  uint32_t * const o32 = output;
  int i;

  assert(byte_count % 4 == 0);

  for (i = 0; i < word_count; i++)
    o32[i] = i32[word_count - i - 1];
}

/*
 * Clone operand into a reversed buffer.  Necessary lack of curly
 * braces makes this unsuitable for use in library code, but it
 * simplifies test setup here.
 */

#define clone_reversed(_clone, _orig)		\
  uint8_t _clone[sizeof(_orig)];		\
  two_card_monty(_clone, _orig, sizeof(_orig))


/* TC0: Read name and version from ModExpS6 core. */
int TC0(void)
{
  uint8_t name0[4]   = { 'm', 'o', 'd', 'e'};
  uint8_t name1[4]   = { 'x', 'p', 's', '6'};
  uint8_t version[4] = { '0', '.', '1', '0'};

  if (!quiet)
    printf("TC0: Reading name and version words from ModExpS6 core.\n");

  return
    tc_expected(MODEXPS6_ADDR_NAME0, name0, sizeof(name0)) ||
    tc_expected(MODEXPS6_ADDR_NAME1, name1, sizeof(name1)) ||
    tc_expected(MODEXPS6_ADDR_VERSION, version, sizeof(version));
}

/* TC1: Fast single 1024-bit message. */
int TC1(void)
{
  int ret;

  if (!quiet)
    printf("TC1: Sign 1024-bit message (fast & unsafe public mode).\n");

  /* Change order of 32-bit words for all the operands (first word becomes last word, and so on...) */
  clone_reversed(modulus,  n_1024);
  clone_reversed(message,  m_1024);
  clone_reversed(exponent, d_1024);
  clone_reversed(result,   s_1024);

  /* Set fast mode */
  /*uint8_t mode_slow_secure[] = {0, 0, 0, 0};*/
  uint8_t mode_fast_unsafe[] = {0, 0, 0, 1};
  tc_write(MODEXPS6_ADDR_MODE, mode_fast_unsafe, sizeof(mode_fast_unsafe));

  /* Set new modulus size */
  tc_width(MODEXPS6_ADDR_MODULUS_WIDTH, sizeof(modulus) * 8);	// number of bits

  /* Write new modulus */
  tc_write(MODEXPS6_ADDR_MODULUS, modulus, sizeof(modulus));

  /* Pre-calculate speed-up coefficient */
  tc_init(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_ready(MODEXPS6_ADDR_STATUS);

  /* Write new message */
  tc_write(MODEXPS6_ADDR_MESSAGE, message, sizeof(message));

  /* Set new exponent length */
  tc_width(MODEXPS6_ADDR_EXPONENT_WIDTH, sizeof(exponent) * 8);	// number of bits

  /* Write new exponent */
  tc_write(MODEXPS6_ADDR_EXPONENT, exponent, sizeof(exponent));

  /* Start calculation */
  tc_next(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_valid(MODEXPS6_ADDR_STATUS);

  /* Compare actual result with expected value */
  ret = tc_expected(MODEXPS6_ADDR_RESULT, result, sizeof(result));

  return ret;
}

/* TC2: Slow single 1024-bit message. */
int TC2(void)
{
  int ret;

  if (!quiet)
    printf("TC2: Sign 1024-bit message (slow & secure private mode).\n");

  /* Change order of 32-bit words for all the operands (first word becomes last word, and so on...) */
  clone_reversed(modulus,  n_1024);
  clone_reversed(message,  m_1024);
  clone_reversed(exponent, d_1024);
  clone_reversed(result,   s_1024);

  /* Set slow mode */
  uint8_t mode_slow_secure[] = {0, 0, 0, 0};
  /*uint8_t mode_fast_unsafe[] = {0, 0, 0, 1};*/
  tc_write(MODEXPS6_ADDR_MODE, mode_slow_secure, sizeof(mode_slow_secure));

  /* Set new modulus size */
  tc_width(MODEXPS6_ADDR_MODULUS_WIDTH, sizeof(modulus) * 8);	// number of bits

  /* Write new modulus */
  tc_write(MODEXPS6_ADDR_MODULUS, modulus, sizeof(modulus));

  /* Pre-calculate speed-up coefficient */
  tc_init(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_ready(MODEXPS6_ADDR_STATUS);

  /* Write new message */
  tc_write(MODEXPS6_ADDR_MESSAGE, message, sizeof(message));

  /* Set new exponent length */
  tc_width(MODEXPS6_ADDR_EXPONENT_WIDTH, sizeof(exponent) * 8);	// number of bits

  /* Write new exponent */
  tc_write(MODEXPS6_ADDR_EXPONENT, exponent, sizeof(exponent));

  /* Start calculation */
  tc_next(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_valid(MODEXPS6_ADDR_STATUS);

  /* Compare actual result with expected value */
  ret = tc_expected(MODEXPS6_ADDR_RESULT, result, sizeof(result));

  return ret;
}

/* TC3: Fast single 2048-bit message. */
int TC3(void)
{
  int ret;

  if (!quiet)
    printf("TC3: Sign 2048-bit message (fast & unsafe public mode).\n");

  /* Change order of 32-bit words for all the operands (first word becomes last word, and so on...) */
  clone_reversed(modulus,  n_2048);
  clone_reversed(message,  m_2048);
  clone_reversed(exponent, d_2048);
  clone_reversed(result,   s_2048);

  /* Set fast mode */
  /*uint8_t mode_slow_secure[] = {0, 0, 0, 0};*/
  uint8_t mode_fast_unsafe[] = {0, 0, 0, 1};
  tc_write(MODEXPS6_ADDR_MODE, mode_fast_unsafe, sizeof(mode_fast_unsafe));

  /* Set new modulus size */
  tc_width(MODEXPS6_ADDR_MODULUS_WIDTH, sizeof(modulus) * 8);	// number of bits

  /* Write new modulus */
  tc_write(MODEXPS6_ADDR_MODULUS, modulus, sizeof(modulus));

  /* Pre-calculate speed-up coefficient */
  tc_init(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_ready(MODEXPS6_ADDR_STATUS);

  /* Write new message */
  tc_write(MODEXPS6_ADDR_MESSAGE, message, sizeof(message));

  /* Set new exponent length */
  tc_width(MODEXPS6_ADDR_EXPONENT_WIDTH, sizeof(exponent) * 8);	// number of bits

  /* Write new exponent */
  tc_write(MODEXPS6_ADDR_EXPONENT, exponent, sizeof(exponent));

  /* Start calculation */
  tc_next(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_valid(MODEXPS6_ADDR_STATUS);

  /* Compare actual result with expected value */
  ret = tc_expected(MODEXPS6_ADDR_RESULT, result, sizeof(result));

  return ret;
}

/* TC4: Slow single 2048-bit message. */
int TC4(void)
{
  int ret;

  if (!quiet)
    printf("TC4: Sign 2048-bit message (slow & secure private mode).\n");

  /* Change order of 32-bit words for all the operands (first word becomes last word, and so on...) */
  clone_reversed(modulus,  n_2048);
  clone_reversed(message,  m_2048);
  clone_reversed(exponent, d_2048);
  clone_reversed(result,   s_2048);

  /* Set slow mode */
  uint8_t mode_slow_secure[] = {0, 0, 0, 0};
  /*uint8_t mode_fast_unsafe[] = {0, 0, 0, 1};*/
  tc_write(MODEXPS6_ADDR_MODE, mode_slow_secure, sizeof(mode_slow_secure));

  /* Set new modulus size */
  tc_width(MODEXPS6_ADDR_MODULUS_WIDTH, sizeof(modulus) * 8);	// number of bits

  /* Write new modulus */
  tc_write(MODEXPS6_ADDR_MODULUS, modulus, sizeof(modulus));

  /* Pre-calculate speed-up coefficient */
  tc_init(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_ready(MODEXPS6_ADDR_STATUS);

  /* Write new message */
  tc_write(MODEXPS6_ADDR_MESSAGE, message, sizeof(message));

  /* Set new exponent length */
  tc_width(MODEXPS6_ADDR_EXPONENT_WIDTH, sizeof(exponent) * 8);	// number of bits

  /* Write new exponent */
  tc_write(MODEXPS6_ADDR_EXPONENT, exponent, sizeof(exponent));

  /* Start calculation */
  tc_next(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_valid(MODEXPS6_ADDR_STATUS);

  /* Compare actual result with expected value */
  ret = tc_expected(MODEXPS6_ADDR_RESULT, result, sizeof(result));

  return ret;
}

/* TC5: Fast single 4096-bit message. */
int TC5(void)
{
  int ret;

  if (!quiet)
    printf("TC5: Sign 4096-bit message (fast & unsafe public mode).\n");

  /* Change order of 32-bit words for all the operands (first word becomes last word, and so on...) */
  clone_reversed(modulus,  n_4096);
  clone_reversed(message,  m_4096);
  clone_reversed(exponent, d_4096);
  clone_reversed(result,   s_4096);

  /* Set fast mode */
  /*uint8_t mode_slow_secure[] = {0, 0, 0, 0};*/
  uint8_t mode_fast_unsafe[] = {0, 0, 0, 1};
  tc_write(MODEXPS6_ADDR_MODE, mode_fast_unsafe, sizeof(mode_fast_unsafe));

  /* Set new modulus size */
  tc_width(MODEXPS6_ADDR_MODULUS_WIDTH, sizeof(modulus) * 8);	// number of bits

  /* Write new modulus */
  tc_write(MODEXPS6_ADDR_MODULUS, modulus, sizeof(modulus));

  /* Pre-calculate speed-up coefficient */
  tc_init(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_ready(MODEXPS6_ADDR_STATUS);

  /* Write new message */
  tc_write(MODEXPS6_ADDR_MESSAGE, message, sizeof(message));

  /* Set new exponent length */
  tc_width(MODEXPS6_ADDR_EXPONENT_WIDTH, sizeof(exponent) * 8);	// number of bits

  /* Write new exponent */
  tc_write(MODEXPS6_ADDR_EXPONENT, exponent, sizeof(exponent));

  /* Start calculation */
  tc_next(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_valid(MODEXPS6_ADDR_STATUS);

  /* Compare actual result with expected value */
  ret = tc_expected(MODEXPS6_ADDR_RESULT, result, sizeof(result));

  return ret;
}

/* TC6: Slow single 4096-bit message. */
int TC6(void)
{
  int ret;

  if (!quiet)
    printf("TC6: Sign 4096-bit message (slow & secure private mode).\n");

  /* Change order of 32-bit words for all the operands (first word becomes last word, and so on...) */
  clone_reversed(modulus,  n_4096);
  clone_reversed(message,  m_4096);
  clone_reversed(exponent, d_4096);
  clone_reversed(result,   s_4096);

  /* Set slow mode */
  uint8_t mode_slow_secure[] = {0, 0, 0, 0};
  /*uint8_t mode_fast_unsafe[] = {0, 0, 0, 1};*/
  tc_write(MODEXPS6_ADDR_MODE, mode_slow_secure, sizeof(mode_slow_secure));

  /* Set new modulus size */
  tc_width(MODEXPS6_ADDR_MODULUS_WIDTH, sizeof(modulus) * 8);	// number of bits

  /* Write new modulus */
  tc_write(MODEXPS6_ADDR_MODULUS, modulus, sizeof(modulus));

  /* Pre-calculate speed-up coefficient */
  tc_init(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_ready(MODEXPS6_ADDR_STATUS);

  /* Write new message */
  tc_write(MODEXPS6_ADDR_MESSAGE, message, sizeof(message));

  /* Set new exponent length */
  tc_width(MODEXPS6_ADDR_EXPONENT_WIDTH, sizeof(exponent) * 8);	// number of bits

  /* Write new exponent */
  tc_write(MODEXPS6_ADDR_EXPONENT, exponent, sizeof(exponent));

  /* Start calculation */
  tc_next(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_valid(MODEXPS6_ADDR_STATUS);

  /* Compare actual result with expected value */
  ret = tc_expected(MODEXPS6_ADDR_RESULT, result, sizeof(result));

  return ret;
}

/* TC7: Signing of multiple 1024-bit messages with same key. */
int TC7(void)
{
  int ret;

  if (!quiet)
    printf("TC7: Sign several 1024-bit messages (without pre-calculation every time).\n");

  /* Change order of 32-bit words for all the operands (first word becomes last word, and so on...) */
  clone_reversed(modulus,   n_1024);
  clone_reversed(exponent,  d_1024);
  clone_reversed(message_0, m_1024_0);
  clone_reversed(message_1, m_1024_1);
  clone_reversed(message_2, m_1024_2);
  clone_reversed(message_3, m_1024_3);
  clone_reversed(result_0,  s_1024_0);
  clone_reversed(result_1,  s_1024_1);
  clone_reversed(result_2,  s_1024_2);
  clone_reversed(result_3,  s_1024_3);

  /* Set fast mode */
  /*uint8_t mode_slow_secure[] = {0, 0, 0, 0};*/
  uint8_t mode_fast_unsafe[] = {0, 0, 0, 1};
  tc_write(MODEXPS6_ADDR_MODE, mode_fast_unsafe, sizeof(mode_fast_unsafe));

  /* Set new modulus size */
  tc_width(MODEXPS6_ADDR_MODULUS_WIDTH, sizeof(modulus) * 8);	// number of bits

  /* Write new modulus */
  tc_write(MODEXPS6_ADDR_MODULUS, modulus, sizeof(modulus));

  /* Pre-calculate speed-up coefficient */
  tc_init(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_ready(MODEXPS6_ADDR_STATUS);

  /* Set new exponent length */
  tc_width(MODEXPS6_ADDR_EXPONENT_WIDTH, sizeof(exponent) * 8);	// number of bits

  /* Write new exponent */
  tc_write(MODEXPS6_ADDR_EXPONENT, exponent, sizeof(exponent));

  {
    /* Write new message #0 */
    tc_write(MODEXPS6_ADDR_MESSAGE, message_0, sizeof(message_0));

    /* Start calculation */
    tc_next(MODEXPS6_ADDR_CTRL);

    /* Wait while core is calculating */
    tc_wait_valid(MODEXPS6_ADDR_STATUS);

    /* Compare actual result with expected value */
    ret = tc_expected(MODEXPS6_ADDR_RESULT, result_0, sizeof(result_0));
    if (ret) return 1;
  }
  {
    /* Write new message #1 */
    tc_write(MODEXPS6_ADDR_MESSAGE, message_1, sizeof(message_1));

    /* Start calculation */
    tc_next(MODEXPS6_ADDR_CTRL);

    /* Wait while core is calculating */
    tc_wait_valid(MODEXPS6_ADDR_STATUS);

    /* Compare actual result with expected value */
    ret = tc_expected(MODEXPS6_ADDR_RESULT, result_1, sizeof(result_1));
    if (ret) return 1;
  }
  {
    /* Write new message #2 */
    tc_write(MODEXPS6_ADDR_MESSAGE, message_2, sizeof(message_2));

    /* Start calculation */
    tc_next(MODEXPS6_ADDR_CTRL);

    /* Wait while core is calculating */
    tc_wait_valid(MODEXPS6_ADDR_STATUS);

    /* Compare actual result with expected value */
    ret = tc_expected(MODEXPS6_ADDR_RESULT, result_2, sizeof(result_2));
    if (ret) return 1;
  }
  {
    /* Write new message #3 */
    tc_write(MODEXPS6_ADDR_MESSAGE, message_3, sizeof(message_3));

    /* Start calculation */
    tc_next(MODEXPS6_ADDR_CTRL);

    /* Wait while core is calculating */
    tc_wait_valid(MODEXPS6_ADDR_STATUS);

    /* Compare actual result with expected value */
    ret = tc_expected(MODEXPS6_ADDR_RESULT, result_3, sizeof(result_3));
    if (ret) return 1;
  }

  return 0;
}

/* TC8: Fast 4096-bit message verification. */
int TC8(void)
{
  int ret;

  if (!quiet)
    printf("TC8: Verify 4096-bit message (fast mode using public exponent).\n");

  /* Change order of 32-bit words for all the operands (first word becomes last word, and so on...) */
  clone_reversed(modulus,  n_4096);
  clone_reversed(message,  s_4096);
  clone_reversed(exponent, e_4096);
  clone_reversed(result,   m_4096);

  /* Set fast mode */
  /*uint8_t mode_slow_secure[] = {0, 0, 0, 0};*/
  uint8_t mode_fast_unsafe[] = {0, 0, 0, 1};
  tc_write(MODEXPS6_ADDR_MODE, mode_fast_unsafe, sizeof(mode_fast_unsafe));

  /* Set new modulus size */
  tc_width(MODEXPS6_ADDR_MODULUS_WIDTH, sizeof(modulus) * 8);	// number of bits

  /* Write new modulus */
  tc_write(MODEXPS6_ADDR_MODULUS, modulus, sizeof(modulus));

  /* Pre-calculate speed-up coefficient */
  tc_init(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_ready(MODEXPS6_ADDR_STATUS);

  /* Write new message */
  tc_write(MODEXPS6_ADDR_MESSAGE, message, sizeof(message));

  /* Set new exponent length */
#if 1
  tc_width(MODEXPS6_ADDR_EXPONENT_WIDTH, 18);	// number of bits
#else
  tc_width(MODEXPS6_ADDR_EXPONENT_WIDTH, 24);	// number of bits
#endif

  /* Write new exponent */
  tc_write(MODEXPS6_ADDR_EXPONENT, exponent, sizeof(exponent));

  /* Start calculation */
  tc_next(MODEXPS6_ADDR_CTRL);

  /* Wait while core is calculating */
  tc_wait_valid(MODEXPS6_ADDR_STATUS);

  /* Compare actual result with expected value */
  ret = tc_expected(MODEXPS6_ADDR_RESULT, result, sizeof(result));

  return ret;
}


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
  tcfp all_tests[] = { TC0, TC1, TC2, TC3, TC4, TC5, TC6, TC7, TC8 };

  char *usage = "Usage: %s [-h] [-d] [-q] [-r] tc...\n";
  int i, j, opt;

  while ((opt = getopt(argc, argv, "h?dqr")) != -1) {
    switch (opt) {
    case 'h':
    case '?':
      printf(usage, argv[0]);
      return EXIT_SUCCESS;
    case 'd':
      tc_set_debug(1);
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
