/*
 * libutil - Yet Another Utility Library
 * Copyright (c) 2012-2015, Christian Wressnegger
 * --
 * This file is part of the library libutil.
 *
 * libutil is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libutil is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "util/util.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <ctype.h>


#ifndef IOTYPE_FILES
#include <sys/stat.h>
#include <sys/types.h>
#endif

const int cmp(const char* const s, ...)
{
	va_list args;
	va_start(args, s);

	const char* x = va_arg(args, char*);
	for (size_t i = 0; x != NULL; i++)
	{
		if (strcmp(s, x) == 0)
		{
			va_end(args);

			ASSERT(i <= INT_MAX);
			return (int) i;
		}
		x = va_arg(args, char*);
	}

	va_end(args);
	return -1;
}


const int cmp2(const char* const s, const char* const needles[])
{
	const char* const* arr = needles;
	for (size_t i = 0; *arr != NULL; i++)
	{
		if (strcmp(s, *arr) == 0)
		{
			ASSERT(i <= INT_MAX);
			return (int) i;
		}
		arr++;
	}
	return -1;
}

const int stricmp(const char* const a, const char* const b)
{
	const char* A = a;
	const char* B = b;

	while (tolower(*A) == tolower(*B) && *A != 0x00)
	{
		A++; B++;
	}
	return (*A - *B);
}

const int isprintable(const char* const s)
{
	for (const char* x = s; *x != 0x00; x++)
	{
		if (!isprint(*x))
		{
			return FALSE;
		}
	}
	return TRUE;
}

const size_t inline_decode(char* s, const size_t len)
{
	char buf[5] = "0x00";

	size_t j = 0;
	for (size_t i = 0; i < len; i++, j++)
	{
		if (s[i] != '%')
		{
			s[j] = s[i];
			continue;
		}

		// write out truncated sequence
		const size_t x = len -i;
		if (x <= 2)
		{
			for (size_t k = 0; k < x; k++, i++, j++)
			{
				s[j] = s[i];
			}
			break;
		}

		buf[2] = s[++i];
		buf[3] = s[++i];

		// is valid encoding?
		if (isxdigit(buf[2]) && isxdigit(buf[3]))
		{
			unsigned int ch;
			sscanf(buf, "%x", (unsigned int*) &ch);
			s[j] = (char) ch;
		}
		else
		{
			s[j++] = '%';
			s[j++] = buf[2];
			s[j]   = buf[3];
		}
	}
	s[j] = 0x00;
	return j;
}

// allocate conservatively
#define ENCODE_ALLOC_RATIO(len) ((len) *3)

const size_t encode(char** out, size_t* outsize, const char* const s, const size_t len)
{
	assert(s != NULL);
	assert(out != NULL);
	char* x = *out;

	size_t dummy = 0;
	size_t xsize = *(outsize == NULL ? &dummy : outsize);
	size_t newsize = ENCODE_ALLOC_RATIO(len) +1;

	if (x == NULL)
	{
		x = (char*) malloc(sizeof(char) *(xsize = newsize));
	}
	else if (xsize < newsize)
	{
		xsize = ENCODE_ALLOC_RATIO(len);
		x = (char*) realloc(x, sizeof(char) *(xsize = newsize));
	}

	char* y = x;
	for (size_t i = 0; i < len; i++)
	{
		if (isprint(s[i]))
		{
			*y = s[i]; y++;
		}
		else
		{
			y += sprintf(y, "%%%02x", s[i]);
		}
	}

	xsize = (size_t) (y -x);
	x = (char*) realloc(x, sizeof(char) *xsize +1);
	*y = 0x00;

	*out = x;
	return xsize;
}

const int starts_with(const char* const s, const char* const prefix)
{
	assert(s != NULL && prefix != NULL);
	return (strstr(s, prefix) == s);
}

char* const ltrim(char* const s)
{
	char* x = s;
	while (isspace(*x) && *x != 0x00) x++;
	return x;
}

char* const rtrim(char* const s)
{
	char* x = s +UNSIGNED_SUBSTRACTION(strlen(s), 1);
	while (isspace(*x) && x != s) x--;
	x[1] = 0x00;
	return x;
}

const size_t count_char(const char* const s, const char ch)
{
	assert(s != NULL);

	const char* x = s -1;
	size_t c = 0;

	while (*(++x) != 0x00)
	{
		if (*x == ch)
		{
			c += 1;
		}
	}
	return c;
}

char* const join_ex(const char* const prefix, const char* const sep, const char** strs, const char* const fmt)
{
	assert(sep != NULL && strs != NULL);

	static const size_t BUF_SIZE = 100;
	char buf[BUF_SIZE];

	size_t fmt_size = 0;
	char* format_string = "%s%s";

	if (fmt != NULL)
	{
		snprintf(buf, BUF_SIZE, fmt, "");
		fmt_size = strlen(buf);

		snprintf(buf, BUF_SIZE, "%%s%s", fmt);
		format_string = buf;
	}


	size_t size = (prefix != NULL ? strlen(prefix) : 0);
	size_t num_strings = 0;
	for (const char** x = strs; *x != NULL; x++)
	{
		size += strlen(*x) +fmt_size;
		num_strings++;
	}

	if (num_strings > 0)
	{
		size += (num_strings -1) * strlen(sep);
	}

	char* const out = (char*) malloc(sizeof(char) * (size +1));
	if (out == NULL)
	{
		return NULL;
	}

	if (size <= 0)
	{
		out[size] = 0x00;
		return out;
	}

	if (num_strings <= 0)
	{
		strcpy(out, prefix);
		return out;
	}

	sprintf(out, format_string, (prefix != NULL ? prefix : ""), *strs);
	char* y = strchr(out, 0x00);

	for (const char** x = strs +1; *x != NULL; x++)
	{
		sprintf(y, format_string, sep, *x);
		y = strchr(y, 0x00);
	}
	return out;
}

char* const join(const char* const sep, const char** strs)
{
	return join_ex(NULL, sep, strs, NULL);
}

void rand_s(char* const out, const size_t n)
{
	static const char alphanum[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";

	for (size_t i = 0; i < n; ++i)
	{
		// rand returns an integer between 0 and RAND_MAX.
		out[i] = alphanum[((size_t) rand()) % (sizeof(alphanum) - 1)];
	}
	out[n] = 0;
}

const int memcmp_bytes(const void* const a, const void* const b, const size_t n)
{
	const uint8_t* A = ((const uint8_t*) a) -1;
	const uint8_t* B = ((const uint8_t*) b) -1;

	const uint8_t* const END = ((const uint8_t*) a) +n;

	while (A < END -1 && *++A == *++B);
	return *A - *B;
}

char* const fread_str(FILE* const f)
{
	// This needs to be at least 4 such that valgrind doesn't complain
	// about strlen reading uninitialized data :/
	size_t n = 4;

	char* out = (char*) malloc((n +1) *sizeof(char));
	char* x = out;

	int c = fgetc(f);
	while (c != EOF && c != 0x00)
	{
		if (x >= out +n)
		{
			out = realloc(out, (n*2 +1) *sizeof(char));
			x = out +n; n *= 2;
		}
		*(x++) = (char) c;
		c = fgetc(f);
	}
	*x = 0x00;
	return realloc(out, (STRNLEN(out, n) +1) *sizeof(char));
}

const float frand()
{
	return ((float) rand()) / ((float) RAND_MAX);
}

extern inline size_t ftell_s(FILE* const f);
extern inline int fseek_s(FILE* const f, const size_t offset, const int whence);

#ifdef IOTYPE_FILES
FILE* const open_file_ex(char* path, char* name, struct stat* st)
{
	assert(path != NULL);
	char file[512];

	if (name != NULL)
	{
		snprintf(file, 512, "%s/%s", path, name);
	}
	else
	{
		snprintf(file, 512, "%s", path);
	}

	// Open file
	FILE* f = fopen(file, "rb");
	if (f == NULL)
	{
		// warning("Could not open file '%s'", file);
		return NULL;
	}

	if (st != NULL)
	{
		stat(file, st);
	}
	return f;
}

char* load_file(char* path, char* name, unsigned int* size)
{
	// Open file
	struct stat st;

	FILE* f = open_file_ex(path, name, &st);
	if (f == NULL)
	{
		return NULL;
	}

	// Allocate memory
	if (size != NULL)
	{
		*size = st.st_size;
	}
	char* x = malloc((st.st_size + 1) * sizeof(char));
	if (x == NULL)
	{
		return NULL;
	}

	// Read data
	long read = fread(x, sizeof(char), st.st_size, f);
	fclose(f);

	if (st.st_size != read)
	{
		// weird things are happening
	}
	return x;
}
#endif

