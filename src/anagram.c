/**
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2013, Christian Wressnegger
 * --
 * This file is part of Letter Salad or Salad for short.
 *
 * Salad is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Salad is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "anagram.h"

#include "hash.h"
#include "util/util.h"

#include <string.h>


hashfunc_t HASH_FCTS[NUM_HASHFCTS] =
{
	sax_hash_n,
	sdbm_hash_n,
	bernstein_hash_n,
	murmur_hash0_n,
	murmur_hash1_n,
	murmur_hash2_n
};

const int to_hashid(hashfunc_t h)
{
	for (int j = 0; j < NUM_HASHFCTS; j++)
	{
		if (HASH_FCTS[j] == h)
		{
			return j;
		}
	}
	return -1;
}


// n-grams

// ATTENTION: Due to the reasonable size of the code for extracting n-grams,
// we accept the code duplicate for the following 3 functions, in order to keep
// the runtime performance of the program high.
void bloomize_ex(BLOOM* const bloom, const char* const str, const size_t len, const size_t n)
{
	const char* x = str;
    for (; x < str + len; x++)
    {
        // Check for sequence end
        if (x + n > str + len) break;

        bloom_add(bloom, x, n);
    }
}

void bloomize_ex2(BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const vec_t* const weights)
{
	const char* x = str;
    for (; x < str + len; x++)
    {
        // Check for sequence end
        if (x + n > str + len) break;

	    const dim_t dim = hash(x, n);
	    if (vec_get(weights, dim) > 0.0)
	    {
	        bloom_add(bloom, x, n);
	    }
    }
}

void bloomize_ex3(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, bloomize_stats_t* const out)
{
	if (out == NULL)
	{
		bloomize_ex(bloom1, str, len, n);
		return;
	}

	out->new = out->uniq = 0;
	out->total = (len >= n ? len -n +1 : 0);
	bloom_clear(bloom2);

	const char* x = str;
    for (; x < str + len; x++)
    {
        // Check for sequence end
        if (x + n > str + len) break;

        if (!bloom_check(bloom1, x, n)) out->new++;
        bloom_add(bloom1, x, n);

        if (!bloom_check(bloom2, x, n)) out->uniq++;
        bloom_add(bloom2, x, n);
    }
}

void bloomize_ex4(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, bloomize_stats_t* const out)
{
	if (out == NULL)
	{
		bloomize_ex(bloom1, str, len, n);
		return;
	}

	out->new = out->uniq = 0;
	out->total = (len >= n ? len -n +1 : 0);
	bloom_clear(bloom2);

	const char* x = str;
    for (; x < str + len; x++)
    {
        // Check for sequence end
        if (x + n > str + len) break;

        if (!bloom_check(bloom1, x, n)) out->new++;
        if (!bloom_check(bloom2, x, n)) out->uniq++;
        bloom_add(bloom2, x, n);
    }
}

const double anacheck_ex(BLOOM* const bloom, const char* const input, const size_t len, const size_t n)
{
	const char* x = input;

	unsigned int numKnown = 0;
	unsigned int numNGrams = 0;

	for (; x < input + len; x++)
	{
		// Check for sequence end
		if (x + n > input + len) break;

		numKnown += bloom_check(bloom, x, n);
		numNGrams++;
	}

	return ((double) (numNGrams -numKnown))/ numNGrams;
}

const double anacheck_ex2(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n)
{
	const char* x = input;

	unsigned int numGoodKnown = 0;
	unsigned int numBadKnown = 0;
	unsigned int numNGrams = 0;

	for (; x < input + len; x++)
	{
		// Check for sequence end
		if (x + n > input + len) break;

		numGoodKnown += bloom_check(bloom, x, n);
		numBadKnown += bloom_check(bbloom, x, n);
		numNGrams++;
	}

	return (((double) (numBadKnown)) -numGoodKnown) /numNGrams;
}


// w-grams
const int pick_delimiterchar(const uint8_t* const delim)
{
	for (size_t i = 0; i < 256; i++)
	{
		if (delim[i])
		{
			return i;
		}
	}
	return -1;
}

char* const uniquify(const char** const str, size_t* const len, const uint8_t* const delim, const int ch)
{
	assert(str != NULL && *str != NULL && len != NULL);

	size_t slen = 0;
	char* s = (char*) malloc(*len +2);

	int prev = ch;
    for (const char* x = *str; x < *str +*len; x++)
    {
		if (delim[(unsigned char) *x])
		{
			if (prev != ch)
			{
				prev = s[slen++] = ch;
			}
		}
		else
		{
			prev = s[slen++] = *x;
		}
    }
    // make sure to end with a separator character
    if (prev != ch)
    {
    	s[slen++] = ch;
    }

    *str = s;
    *len = slen;
    return s;
}


typedef void(*FN_PROCESS_WGRAM)(const char* const wgram, const size_t len, void* const data);


void extract_wgrams(const char* const str, const size_t len, const size_t n, const uint8_t* const delim, FN_PROCESS_WGRAM fct, void* const data)
{
	int ch = pick_delimiterchar(delim);

	const char* s = str;
	size_t slen = len;
	// ATTENTION! "uniquify" allocates new memory stored in &s
	uniquify(&s, &slen, delim, ch);

    const char** wgrams = (const char**) malloc(sizeof(char*) *(n +1));
    wgrams[0] = s;

    // Initialize sliding window
	const char* x = s;
    for (int i = 1; x < s +slen && i < n; x++)
    {
    	if (*x == ch)
    	{
    		wgrams[i++] = x +1;
    	}
    }

    int i = n -1, a = n +1;
    for (; x < s +slen; x++)
    {
    	if (*x == ch)
    	{
    		i = (i+1) % a;
    		wgrams[i] = x +1;

    		const int y = (i +1) % a;
    		const char* const next = wgrams[y];
    		const size_t wlen =  wgrams[i] -next -1;

    		fct(next, wlen, data);
    	}
    }

    free(wgrams);
    free((void*) s);
}

typedef struct
{
	BLOOM* bloom;
	const vec_t* weights;

} bloomize_t;

typedef struct
{
	BLOOM* bloom1;
	BLOOM* bloom2;
	size_t new, uniq, total;
} blommize_stats_ex_t;


void simple_add(const char* const wgram, const size_t len, void* const data)
{
	assert(wgram != NULL && data != NULL);
	bloomize_t* const d = (bloomize_t*) data;

	bloom_add(d->bloom, wgram, len);
}

void checked_add(const char* const wgram, const size_t len, void* const data)
{
	assert(wgram != NULL && data != NULL);
	bloomize_t* const d = (bloomize_t*) data;

    const dim_t dim = hash(wgram, len);
    if (vec_get(d->weights, dim) > 0.0)
    {
		bloom_add(d->bloom, wgram, len);
    }
}

void counted_add(const char* const wgram, const size_t len, void* const data)
{
	assert(wgram != NULL && data != NULL);
	blommize_stats_ex_t* const d = (blommize_stats_ex_t*) data;

	if (!bloom_check(d->bloom1, wgram, len))
	{
		d->new++;
	}
	if (!bloom_check(d->bloom2, wgram, len))
	{
		d->uniq++;
	}
	d->total++;

	bloom_add(d->bloom1, wgram, len);
	bloom_add(d->bloom2, wgram, len);
}

void count(const char* const wgram, const size_t len, void* const data)
{
	assert(wgram != NULL && data != NULL);
	blommize_stats_ex_t* const d = (blommize_stats_ex_t*) data;

	if (!bloom_check(d->bloom1, wgram, len))
	{
		d->new++;
	}
	if (!bloom_check(d->bloom2, wgram, len))
	{
		d->uniq++;
	}
	d->total++;

	bloom_add(d->bloom2, wgram, len);
}

void bloomizew_ex(BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const uint8_t* const delim)
{
	bloomize_t data;
	data.bloom = bloom;
	data.weights = NULL;

	extract_wgrams(str, len, n, delim, simple_add, &data);
}

void bloomizew_ex2(BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const uint8_t* const delim, const vec_t* const weights)
{
	bloomize_t data;
	data.bloom = bloom;
	data.weights = weights;

	extract_wgrams(str, len, n, delim, checked_add, &data);
}

void bloomizew_ex3(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, const uint8_t* const delim, bloomize_stats_t* const out)
{
	if (out == NULL)
	{
		bloomizew_ex(bloom1, str, len, n, delim);
		return;
	}

	blommize_stats_ex_t data;
	data.bloom1 = bloom1;
	data.bloom2 = bloom2;
	data.new = data.uniq = data.total = 0;

	bloom_clear(bloom2);
	extract_wgrams(str, len, n, delim, counted_add, &data);

	out->new = data.new;
	out->uniq = data.uniq;
	out->total = data.total;
}

void bloomizew_ex4(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, const uint8_t* const delim, bloomize_stats_t* const out)
{
	if (out == NULL)
	{
		bloomizew_ex(bloom1, str, len, n, delim);
		return;
	}

	blommize_stats_ex_t data;
	data.bloom1 = bloom1;
	data.bloom2 = bloom2;
	data.new = data.uniq = data.total = 0;

	bloom_clear(bloom2);
	extract_wgrams(str, len, n, delim, count, &data);

	out->new = data.new;
	out->uniq = data.uniq;
	out->total = data.total;
}

typedef struct
{
	BLOOM* bloom;
	unsigned int numKnown;
	unsigned int numNGrams;

} anacheck_t;

void check(const char* const wgram, const size_t len, void* const data)
{
	assert(wgram != NULL && data != NULL);
	anacheck_t* const d = (anacheck_t*) data;

	d->numKnown += bloom_check(d->bloom, wgram, len);
	d->numNGrams++;
}

#define GOOD 0
#define BAD  1

void check2(const char* const wgram, const size_t len, void* const data)
{
	assert(wgram != NULL && data != NULL);
	anacheck_t* const d = (anacheck_t*) data;

	d[GOOD].numKnown += bloom_check(d[GOOD].bloom, wgram, len);
	d[BAD ].numKnown += bloom_check(d[BAD ].bloom, wgram, len);
	d[BAD ].numNGrams++;
}

const double anacheckw_ex(BLOOM* const bloom, const char* const input, const size_t len, const size_t n, const uint8_t* const delim)
{
	anacheck_t data;
	data.bloom = bloom;
	data.numKnown = 0;
	data.numNGrams = 0;

	extract_wgrams(input, len, n, delim, check, &data);
	return ((double) (data.numNGrams -data.numKnown))/ data.numNGrams;
}

const double anacheckw_ex2(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n, const uint8_t* const delim)
{

	anacheck_t data[2];
	data[GOOD].bloom = bloom;
	data[GOOD].numKnown = 0;
	data[GOOD].numNGrams = 0;

	data[BAD].bloom = bbloom;
	data[BAD].numKnown = 0;
	data[BAD].numNGrams = 0;

	extract_wgrams(input, len, n, delim, check2, &data);
	return (((double)data[BAD].numKnown) -data[GOOD].numKnown)/ data[BAD].numNGrams;
}


BLOOM* const bloom_init(const unsigned short size, const hashset_t hs)
{
	assert(size <= sizeof(void*) *8);

	switch (hs)
	{
	case HASHES_SIMPLE:
		return bloom_create_ex(POW(2, size), HASHSET_SIMPLE);

	case HASHES_MURMUR:
		return bloom_create_ex(POW(2, size), HASHSET_MURMUR);

	default: break;
	}
	return NULL;
}


BLOOM* const bloom_init_from_file(FILE* const f)
{
	uint8_t nfuncs;
	hashfunc_t* hashfuncs;
	if (fread_hashspec(f, &hashfuncs, &nfuncs) < 0) return NULL;

	BLOOM* const b = bloom_create_from_file_ex(f, hashfuncs, nfuncs);
	free(hashfuncs);
	return b;
}

BLOOM* const bloomize(const char* str, const size_t len, const size_t n)
{
	BLOOM* const bloom = bloom_init(DEFAULT_BFSIZE, HASHES_SIMPLE);
	if (bloom != NULL)
	{
		bloomize_ex(bloom, str, len, n);
	}
	return bloom;
}

BLOOM* const bloomizew(const char* str, const size_t len, const size_t n, const uint8_t* const delim)
{
	BLOOM* const bloom = bloom_init(DEFAULT_BFSIZE, HASHES_SIMPLE);
	if (bloom != NULL)
	{
		bloomizew_ex(bloom, str, len, n, delim);
	}
	return bloom;
}

const int fwrite_hashspec(FILE* const f, BLOOM* const bloom)
{
	assert(f != NULL);

	if (fwrite(&bloom->nfuncs, sizeof(uint8_t), 1, f) != 1) return -1;

	for (uint8_t i = 0; i < bloom->nfuncs; i++)
	{
		const int id = to_hashid(bloom->funcs[i]);
		if (id < 0 || id >= 256) return -1;

		if (fwrite(&id, sizeof(uint8_t), 1, f) != 1) return -1;
	}

	return 1 +bloom->nfuncs;
}

const int fwrite_model(FILE* const f, BLOOM* const bloom, const size_t ngramLength, const char* const delimiter)
{
	const char* const x = (delimiter == NULL ? "" : delimiter);

	size_t n = strlen(x);
	int m, l;
	if (fwrite(x, sizeof(char), n +1, f) != n +1) return -1;

	if (fwrite(&ngramLength, sizeof(size_t), 1, f) != 1) return -1;

	if ((m = fwrite_hashspec(f, bloom)) < 0) return -1;

	if ((l = bloom_to_file(bloom, f)) < 0) return -1;

	return n *sizeof(char) +sizeof(size_t) +m +l;
}

const int fread_hashspec(FILE* const f, hashfunc_t** const hashfuncs, uint8_t* const nfuncs)
{
	*nfuncs = 0;
	uint8_t fctid = 0xFF;

	const size_t nread = fread(nfuncs, sizeof(uint8_t), 1, f);
	if (nread != 1) return -1;

	*hashfuncs = (hashfunc_t*) calloc(*nfuncs, sizeof(hashfunc_t));
	if (*hashfuncs == NULL)
	{
		return -1;
	}
	for (int i = 0; i < (int) *nfuncs; i++)
	{
		if (fread(&fctid, sizeof(uint8_t), 1, f) != 1 || fctid >= NUM_HASHFCTS)
		{
			free(*hashfuncs);
			*hashfuncs = NULL;
			return -1;
		}
		(*hashfuncs)[i] = HASH_FCTS[fctid];
	}
	return nread;
}

const int fread_model(FILE* const f, BLOOM** const bloom, size_t* const ngramLength, uint8_t* const delim, int* const useWGrams)
{
	if (useWGrams != NULL) *useWGrams = 0;

	char* const delimiter = fread_str(f);
	if (delimiter != NULL)
	{
		if (delimiter[0] != 0x00)
		{
			if (useWGrams != NULL) *useWGrams = 1;
			if (delim != NULL) to_delimiter_array(delim, delimiter);
		}
		free(delimiter);
	}

	// n-gram length
	size_t dummy;
	size_t* const _ngramLength = (ngramLength == NULL ? &dummy : ngramLength);

	*_ngramLength = 0;
	size_t numRead = fread(_ngramLength, sizeof(size_t), 1, f);

	// The actual bloom filter values
	*bloom = bloom_init_from_file(f);

	return (numRead <= 0 || *_ngramLength <= 0 || *bloom == NULL ? -1 : 0);
}

void to_delimiter_array(uint8_t* const out, const char* const s)
{
	assert(out != NULL);
    memset(out, 0, 256);

	if (s == NULL)
	{
		return;
	}

	char* const x = malloc((strlen(s) +1) *sizeof(char));
	strcpy(x, s);
	inline_decode(x, strlen(x));

    for (size_t i = 0; i < strlen(x); i++)
    {
    	out[(unsigned int) x[i]] = 1;
    }
    free(x);
}
