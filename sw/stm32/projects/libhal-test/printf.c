/*****************************************************************************
Stripped-down printf()
Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: Dec 12, 2003
This code is public domain (no copyright).
You can do whatever you want with it.

Revised Dec 12, 2003
- fixed vsprintf() and sprintf() in test code

Revised Jan 28, 2002
- changes to make characters 0x80-0xFF display properly

Revised June 10, 2001
- changes to make vsprintf() terminate string with '\0'

Revised May 12, 2000
- math in DO_NUM is now unsigned, as it should be
- %0 flag (pad left with zeroes) now works
- actually did some TESTING, maybe fixed some other bugs

%[flag][width][.prec][mod][conv]
flag:	-	left justify, pad right w/ blanks	DONE
	0	pad left w/ 0 for numerics		DONE
	+	always print sign, + or -		no
	' '	(blank)					no
	#	(???)					no

width:		(field width)				DONE

prec:		(precision)				DONE

conv:	d,i	decimal int				DONE
	u	decimal unsigned			DONE
	o	octal					DONE
	x,X	hex					DONE
	f,e,g,E,G float					no
	c	char					DONE
	s	string					DONE
	p	ptr					DONE

mod:	N	near ptr				DONE
	F	far ptr					no
	h	short (16-bit) int			DONE
	l	long (32-bit) int			DONE
	L/ll	long long (64-bit) int			no
*****************************************************************************/
#include <string.h> /* strlen() */
#include <stdio.h> /* stdout, putchar(), fputs() (but not printf() :) */
#undef putchar

#include <stdarg.h> /* va_list, va_start(), va_arg(), va_end() */

/* flags used in processing format string */
#define	PR_LJ	0x01	/* left justify */
#define	PR_CA	0x02	/* use A-F instead of a-f for hex */
#define	PR_SG	0x04	/* signed numeric conversion (%d vs. %u) */
#define	PR_32	0x08	/* long (32-bit) numeric conversion */
#define	PR_16	0x10	/* short (16-bit) numeric conversion */
#define	PR_WS	0x20	/* PR_SG set and num was < 0 */
#define	PR_LZ	0x40	/* pad left with '0' instead of ' ' */
#define	PR_FP	0x80	/* pointers are far */
/* largest number handled is 2^64-1, lowest radix handled is 8.
2^64-1 in base 8 has 22 digits (add 2 for trailing NUL and for slop) */
#define	PR_BUFLEN	24

#ifndef max
#define max(a,b) ((a > b) ? a : b)
#endif

typedef int (*fnptr_t)(unsigned c, void **helper);
/*****************************************************************************
name:	do_printf
action:	minimal subfunction for ?printf, calls function
	'fn' with arg 'ptr' for each character to be output
returns:total number of characters output
*****************************************************************************/
int do_printf(const char *fmt, va_list args, fnptr_t fn, void *ptr)
{
        unsigned flags, length, count, width, precision;
	unsigned char *where, buf[PR_BUFLEN];
	unsigned char state, radix;
	long num;

	state = flags = count = width = precision = 0;
/* begin scanning format specifier list */
	for(; *fmt; fmt++)
	{
		switch(state)
		{
/* STATE 0: AWAITING % */
		case 0:
			if(*fmt != '%')	/* not %... */
			{
				fn(*fmt, &ptr);	/* ...just echo it */
				count++;
				break;
			}
/* found %, get next char and advance state to check if next char is a flag */
			state++;
			fmt++;
			/* FALL THROUGH */
/* STATE 1: AWAITING FLAGS (%-0) */
		case 1:
			if(*fmt == '%')	/* %% */
			{
				fn(*fmt, &ptr);
				count++;
				state = flags = width = precision = 0;
				break;
			}
			if(*fmt == '-')
			{
				if(flags & PR_LJ)/* %-- is illegal */
					state = flags = width = precision = 0;
				else
					flags |= PR_LJ;
				break;
			}
/* not a flag char: advance state to check if it's field width */
			state++;
/* check now for '%0...' */
			if(*fmt == '0')
			{
				flags |= PR_LZ;
				fmt++;
			}
			/* FALL THROUGH */
/* STATE 2: AWAITING (NUMERIC) FIELD WIDTH */
		case 2:
			if(*fmt >= '0' && *fmt <= '9')
			{
				width = 10 * width + (*fmt - '0');
				break;
			}
/* not a field width: advance state to check if it's field precision */
			state++;
			/* FALL THROUGH */
/* STATE 3: AWAITING (NUMERIC) FIELD PRECISION */
		case 3:
			if(*fmt == '.')
				++fmt;
			if(*fmt >= '0' && *fmt <= '9')
			{
				precision = 10 * precision + (*fmt - '0');
				break;
			}
/* not field precision: advance state to check if it's a modifier */
			state++;
			/* FALL THROUGH */
/* STATE 4: AWAITING MODIFIER CHARS (FNlh) */
		case 4:
			if(*fmt == 'F')
			{
				flags |= PR_FP;
				break;
			}
			if(*fmt == 'N')
				break;
			if(*fmt == 'l')
			{
				flags |= PR_32;
				break;
			}
			if(*fmt == 'h')
			{
				flags |= PR_16;
				break;
			}
/* not modifier: advance state to check if it's a conversion char */
			state++;
			/* FALL THROUGH */
/* STATE 5: AWAITING CONVERSION CHARS (Xxpndiuocs) */
		case 5:
			where = buf + PR_BUFLEN - 1;
			*where = '\0';
			switch(*fmt)
			{
			case 'X':
				flags |= PR_CA;
				/* FALL THROUGH */
/* xxx - far pointers (%Fp, %Fn) not yet supported */
			case 'x':
			case 'p':
			case 'n':
				radix = 16;
				goto DO_NUM;
			case 'd':
			case 'i':
				flags |= PR_SG;
				/* FALL THROUGH */
			case 'u':
				radix = 10;
				goto DO_NUM;
			case 'o':
				radix = 8;
/* load the value to be printed. l=long=32 bits: */
DO_NUM:				if(flags & PR_32)
					num = va_arg(args, unsigned long);
/* h=short=16 bits (signed or unsigned) */
				else if(flags & PR_16)
				{
					num = va_arg(args, int);
					if(flags & PR_SG)
					    num = (short) num;
					else
					    num = (unsigned short) num;
				}
/* no h nor l: sizeof(int) bits (signed or unsigned) */
				else
				{
					if(flags & PR_SG)
						num = va_arg(args, int);
					else
						num = va_arg(args, unsigned int);
				}
/* take care of sign */
				if(flags & PR_SG)
				{
					if(num < 0)
					{
						flags |= PR_WS;
						num = -num;
					}
				}
/* convert binary to octal/decimal/hex ASCII
OK, I found my mistake. The math here is _always_ unsigned */
				do
				{
					unsigned long temp;

					temp = (unsigned long)num % radix;
					where--;
					if(temp < 10)
						*where = temp + '0';
					else if(flags & PR_CA)
						*where = temp - 10 + 'A';
					else
						*where = temp - 10 + 'a';
					num = (unsigned long)num / radix;
				}
				while(num != 0);
/* for integers, precision functions like width, but pads with zeros */
				if (precision != 0)
				{
					width = max(width, precision);
					if (precision > strlen((const char *)where))
						flags |= PR_LZ;
					precision = 0;
				}
				goto EMIT;
			case 'c':
/* disallow pad-left-with-zeroes for %c */
				flags &= ~PR_LZ;
				where--;
				*where = (unsigned char)va_arg(args, int);
				length = 1;
				goto EMIT2;
			case 's':
/* disallow pad-left-with-zeroes for %s */
				flags &= ~PR_LZ;
				where = va_arg(args, unsigned char *);
EMIT:
				length = strlen((const char *)where);
/* if string is longer than precision, truncate */
				if ((precision != 0) && (precision < length))
				{
					length = precision;
					precision = 0;
				}
				if(flags & PR_WS)
					length++;
/* if we pad left with ZEROES, do the sign now */
				if((flags & (PR_WS | PR_LZ)) ==
					(PR_WS | PR_LZ))
				{
					fn('-', &ptr);
					count++;
				}
/* pad on left with spaces or zeroes (for right justify) */
EMIT2:				if((flags & PR_LJ) == 0)
				{
					while(width > length)
					{
						fn(flags & PR_LZ ?
							'0' : ' ', &ptr);
						count++;
						width--;
					}
				}
/* if we pad left with SPACES, do the sign now */
				if((flags & (PR_WS | PR_LZ)) == PR_WS)
				{
					fn('-', &ptr);
					count++;
				}
/* emit string/char/converted number */
				for(unsigned i = (flags & PR_WS) ? 1 : 0;
				    i < length; ++i)
				{
					fn(*where++, &ptr);
					count++;
				}
/* pad on right with spaces (for left justify) */
				if(width < length)
					width = 0;
				else width -= length;
				for(; width; width--)
				{
					fn(' ', &ptr);
					count++;
				}
				break;
			default:
				break;
			}
		default:
			state = flags = width = precision = 0;
			break;
		}
	}
	return count;
}

/*****************************************************************************
SPRINTF
*****************************************************************************/
static int vsprintf_help(unsigned c, void **ptr)
{
	char *dst;

	dst = *ptr;
	*dst++ = (char)c;
	*ptr = dst;
	return 0 ;
}
/*****************************************************************************
*****************************************************************************/
int vsprintf(char *buf, const char *fmt, va_list args)
{
	int rv;

	rv = do_printf(fmt, args, vsprintf_help, (void *)buf);
	buf[rv] = '\0';
	return rv;
}
/*****************************************************************************
*****************************************************************************/
int sprintf(char *buf, const char *fmt, ...)
{
	va_list args;
	int rv;

	va_start(args, fmt);
	rv = vsprintf(buf, fmt, args);
	va_end(args);
	return rv;
}
/*****************************************************************************
PRINTF
You must write your own putchar()
*****************************************************************************/
int vprintf_help(unsigned c, void **ptr)
{
	ptr = ptr;

	putchar(c);
	return 0 ;
}
/*****************************************************************************
*****************************************************************************/
int vprintf(const char *fmt, va_list args)
{
	return do_printf(fmt, args, vprintf_help, NULL);
}
/*****************************************************************************
*****************************************************************************/
int printf(const char *fmt, ...)
{
	va_list args;
	int rv;

	va_start(args, fmt);
	rv = vprintf(fmt, args);
	va_end(args);
	return rv;
}

#if 0 /* testing */
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	char buf[64];

	sprintf(buf, "%lu score and %i years ago...\n", 4L, -7);
	fputs(buf, stdout); /* puts() adds newline */

	sprintf(buf, "-1L == 0x%lX == octal %lo\n", -1L, -1L);
	fputs(buf, stdout); /* puts() adds newline */

	printf("<%-8s> and <%8s> justified strings\n", "left", "right");

	printf("short signed: %hd, short unsigned: %hu\n\n", -1, -1);

    char str[] = "abcdefghijklmnopqrstuvwxyz";
    printf("%8s\n", str);	/* abcdefghijklmnopqrstuvwxyz */
    printf("%8.32s\n", str);	/* abcdefghijklmnopqrstuvwxyz */
    printf("%8.16s\n", str);	/* abcdefghijklmnop */
    printf("%8.8s\n", str);	/* abcdefgh */
    printf("%8.4s\n", str);	/*     abcd */
    printf("%4.8s\n", str);	/* abcdefgh */
    printf("%.8s\n", str);	/* abcdefgh */
    printf("%8s\n", "abcd");	/*     abcd */
    printf("%8.8s\n", "abcd");	/*     abcd */
    printf("%4.8s\n", "abcd");	/* abcd */
    printf("%.8s\n", "abcd");	/* abcd */
    printf("%.0s\n", str);	/* */
    printf("\n");

    int num = 123456;
    printf("%016d\n", num);	/* 0000000000123456 */
    printf("%16d\n", num);	/*           123456 */
    printf("%8d\n", num);	/*   123456 */
    printf("%8.16d\n", num);	/* 0000000000123456 */
    printf("%8.8d\n", num);	/* 00123456 */
    printf("%8.4d\n", num);	/*   123456 */
    printf("%4.4d\n", num);	/* 123456 */
    printf("%.16d\n", num);	/* 0000000000123456 */ 
    printf("%.8d\n", num);	/* 00123456 */
    printf("%.4d\n", num);	/* 123456 */

	return 0;
}
#else

/*****************************************************************************
2015-10-29 pselkirk for cryptech
*****************************************************************************/
/* gcc decides that a plain string with no formatting is best handled by puts() */
int puts(const char *s)
{
    return printf("%s\n", s);
}

/* transmit characters to the uart */
#include "stm-uart.h"
int putchar(int c)
{
    if (c == '\n')
	uart_send_char('\r');
    uart_send_char((uint8_t) c);
    return c;
}
#endif
