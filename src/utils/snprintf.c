/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\brief Simple implementation of snprintf only supporing a small subset of format characters that is guaranteed not to use malloc
#include "strus/private/snprintf.h"
#include <math.h>

static void printnum( char** bi, char* be, unsigned long num)
{
	char buf[ 64];
	size_t bufidx = sizeof(buf);
	if (num == 0)
	{
		if (*bi < be)
		{
			**bi = '0';
			++*bi;
		}
		return;
	}
	buf[ --bufidx] = 0;
	while (num > 0)
	{
		buf[ --bufidx] = (num % 10) + '0';
		num /= 10;
	}
	for (; *bi < be && buf[ bufidx]; ++bufidx,++*bi) **bi = buf[ bufidx];
}

static void printfloat( char** bi, char* be, double num, unsigned int precision)
{
	double fl = floor( num);
	double gz = (num < 0.0)?(fl+1.0):fl;                 /* ganzzahliger teil: -1.3 => -1.0, 0.7 => 0.0, 2.1 => 2.0 */
	double tr = (num < 0.0)?(gz - num):(num - fl);       /* trunc:  -1.3 => 0.3, 0.7 => 0.7, 2.1 => 0.1 */

	if (*bi >= be) return;
	if (num < 0.0)
	{
		**bi = '-';
		++*bi;
		gz = -gz;
	}
	printnum( bi, be, (unsigned long)gz);
	if (*bi < be && precision)
	{
		**bi = '.';
		++*bi;
	}
	for (; *bi < be && precision > 0; ++*bi,--precision)
	{
		tr = tr * 10;
		double trfl = floor(tr);
		tr -= trfl;
		**bi = (char)(unsigned char)(unsigned int)(trfl) + '0';
	}
}

void strus_vsnprintf( char* bi, size_t bufsize, const char* format, va_list ap)
{
	if (bufsize == 0) return;
	char* be = bi + bufsize-1;
	char const* fi = format;

	union
	{
		char* s;
		int d;
		unsigned int u;
		char c;
		double f;
	} val;

	for (; *fi && bi < be; ++fi)
	{
		if (*fi == '%')
		{
			++fi;
			switch (*fi)
			{
				case '%':
					*bi++ = '%';
					break;
				case 'f':
				{
					val.f = va_arg(ap, double);
					printfloat( &bi, be, val.f, 8);
					break;
				}
				case '.':
				{
					char const* fibk = fi++;
					if (*fi < '0' || *fi > '9')
					{
						*bi++ = '.';
						break;
					}
					unsigned int precision = atoi(fi);
					for (; *fi >= '0' && *fi <= '9'; ++fi){}
					if (*fi == 'f')
					{
						val.f = va_arg(ap, double);
						printfloat( &bi, be, val.f, precision);
					}
					else
					{
						*bi++ = '.';
						fi = fibk;
					}
					break;
				}
				case 's':
					val.s = va_arg(ap, char *);
					if (val.s) for (; bi<be && *val.s; ++bi,++val.s) *bi = *val.s;
					break;
				case 'd':
					val.d = va_arg(ap, int);
					if (val.d < 0)
					{
						*bi++ = '-';
						printnum( &bi, be, -val.d);
					}
					else
					{
						printnum( &bi, be, val.d);
					}
					break;
				case 'u':
					val.u = va_arg(ap, unsigned int);
					printnum( &bi, be, val.u);
					break;
				case 'c':
					val.c = (char) va_arg(ap, int);
					*bi++ = val.c;
					break;
			}
		}
		else
		{
			*bi++ = *fi;
		}
	}
	*bi = 0;
}

void strus_snprintf( char* bi, size_t bufsize, const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	strus_vsnprintf( bi, bufsize, format, ap);
	va_end(ap);
}



