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

/**
 * @file
 */

#ifndef UTIL_UTIL_H_
#define UTIL_UTIL_H_

#include <util/config.h>

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef IOTYPE_FILES
#include <dirent.h>
#endif


const int cmp(const char* const s, ...);
const int cmp2(const char* const s, const char* const needles[]);

const int stricmp(const char* const a, const char* const b);

const int isprintable(const char* const s);
const size_t inline_decode(char* s, const size_t len);
const size_t encode(char** out, size_t* outsize, const char* const s, const size_t len);
const int starts_with(const char* const s, const char* const prefix);
char* const ltrim(char* const s);
char* const rtrim(char* const s);
const size_t count_char(const char* const s, const char ch);
char* const join(const char* const sep, const char** strs);
char* const join_ex(const char* const prefix, const char* const sep, const char** strs, const char* const fmt);
void rand_s(char* const out, const size_t n);
const int memcmp_bytes(const void* const a, const void* const b, const size_t n);


#ifdef IOTYPE_FILES
char* load_file(char* path, char* name, unsigned int* size);
#endif

char* const fread_str(FILE* const f);
const float frand();

#if __STDC_VERSION__ >= 199901L
#include <tgmath.h>
#define POW pow
#else
#include <math.h>
#define POW (size_t) pow
#endif
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) < (Y) ? (Y) : (X))

#define UNSIGNED_SUBSTRACTION(A, B) ((A) < (B) ? 0 : (A) - (B))

#if (_XOPEN_SOURCE -0) >= 700
	#define STRDUP(from, to)                               \
			{	                                           \
				to = strdup(from);                         \
			}

	#define STRNDUP(n, from, to)                           \
			{	                                           \
				to = strndup(from, n);                     \
			}  // copies n +1

	#define STRNLEN(s, n) strnlen(s, n)
#else
	#define STRDUP(from, to)                               \
			{	                                           \
				const char* const _from_ = from;           \
				char** _to_ = &(to);                       \
				                                           \
				*_to_ = (char*) malloc(strlen(_from_) +1); \
				strcpy(*_to_, _from_);                     \
			}

	#define STRNDUP(n, from, to)                           \
			{	                                           \
				const size_t _n_ = n;                      \
				const char* const _from_ = from;           \
				char** _to_ = &(to);                       \
				                                           \
				*_to_ = (char*) malloc(_n_ +1);            \
				strncpy(*_to_, _from_, _n_);               \
				(*_to_)[_n_] = 0x00;                       \
			}

	// simply ignore the max-length parameter :/
	#define STRNLEN(s, n) strlen(s)
#endif


inline size_t ftell_s(FILE* const f)
{
	// TODO: Do something clever here to potentially handle 4GB+ files
	const long int pos = ftell(f);
	assert(pos >= 0);
	return (size_t) MAX(0, pos);
}

inline int fseek_s(FILE* const f, const size_t offset, const int whence)
{
	// TODO: Do something clever here to potentially handle 4GB+ files
	const size_t __offset = MIN(LONG_MAX, offset);
	assert(__offset == offset);
	const int ret = fseek(f, (long int) __offset, whence);
	return ret;
}

#define FALSE 0
#define TRUE  1
typedef int BOOL;

#define UNUSED(x) (void)(x)

#ifdef HAS_Z_MODIFIER
	#define Z "z"
	#define ZU "zu"
	#define ZD "zd"
	#define SIZE_T size_t
	#define SSIZE_T ssize_t
#else
	#define Z "l"
	#define ZU "lu"
	#define ZD "ld"
	#define SIZE_T unsigned long
	#define SSIZE_T long
#endif

#ifdef HAS_L_MODIFIER
	#define LF "Lf"
	#define LONG_DOUBLE long double
#else
	#define LF "g"
	#define LONG_DOUBLE double
#endif

# define ASSERT(expr) \
{ \
	assert(expr); \
	if (!(expr)) \
	{ \
		fprintf(stderr, "Assertion failed: %s", #expr); \
		fprintf(stderr, "in %s line %ud", __FILE__, __LINE__); \
		exit(666); \
	} \
}


#define REVERSE_UINT32(x) ( \
		((x) & 0x000000FFU) << 24 | \
		((x) & 0x0000FF00U) <<  8 | \
		((x) & 0x00FF0000U) >>  8 | \
		((x) & 0xFF000000U) >> 24 \
	)

#define REVERSE_UINT64(x) ( \
		((x) & 0x00000000000000FFU) << 56 | \
		((x) & 0x000000000000FF00U) << 40 | \
		((x) & 0x0000000000FF0000U) << 24 | \
		((x) & 0x00000000FF000000U) <<  8 | \
		((x) & 0x000000FF00000000U) >>  8 | \
		((x) & 0x0000FF0000000000U) >> 24 | \
		((x) & 0x00FF000000000000U) >> 40 | \
		((x) & 0xFF00000000000000U) >> 56 \
	)

#if SIZE_MAX == 0xffffffff
#define REVERSE_REG(r) REVERSE_UINT32(r)
#elif SIZE_MAX == 0xffffffffffffffff
#define REVERSE_REG(r) REVERSE_UINT64(r)
#else
		error("unknown architecture");
#endif

#endif /* UTIL_UTIL_H_ */
