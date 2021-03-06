/*
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2015, Christian Wressnegger
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

#include "main.h"

#include <salad/salad.h>
#include <salad/analyze.h>
#include <salad/classify.h>
#include <salad/util.h>
#include <util/log.h>

#include <inttypes.h>
#include <string.h>
#include <math.h>

void bloomizeb_ex3_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomizeb_ex3(p->bloom1, p->bloom2, str, len, p->n, out);
}

void bloomize_ex3_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomize_ex3(p->bloom1, p->bloom2, str, len, p->n, out);
}

void bloomizew_ex3_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomizew_ex3(p->bloom1, p->bloom2, str, len, p->n, p->delim, out);
}

void bloomizeb_ex4_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomizeb_ex4(p->bloom1, p->bloom2, str, len, p->n, out);
}

void bloomize_ex4_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomize_ex4(p->bloom1, p->bloom2, str, len, p->n, out);
}

void bloomizew_ex4_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomizew_ex4(p->bloom1, p->bloom2, str, len, p->n, p->delim, out);
}


FN_BLOOMIZE pick_wrapper(const model_type_t t, const int use_new)
{
	switch (t)
	{
	case BIT_NGRAM:
		return (use_new ? bloomizeb_ex3_wrapper : bloomizeb_ex4_wrapper);

	case BYTE_NGRAM:
		return (use_new ? bloomize_ex3_wrapper  : bloomize_ex4_wrapper );

	case TOKEN_NGRAM:
		return (use_new ? bloomizew_ex3_wrapper : bloomizew_ex4_wrapper);
	}
	return NULL;
}

typedef struct {
	FN_BLOOMIZE fct;
	bloom_param_t param;

	char buf[0x100];
	bloomize_stats_t* stats;
	size_t num_uniq;
	FILE* const out;
} inspect_t;

const int salad_inspect_callback(data_t* data, const size_t n, void* const usr)
{
	inspect_t* const x = (inspect_t*) usr;

	for (size_t i = 0; i < n; i++)
	{
		x->fct(&x->param, data[i].buf, data[i].len, &x->stats[i]);
	}

	for (size_t i = 0; i < n; i++)
	{
		snprintf(x->buf, 0x100, "%10"ZU"\t%10"ZU"\t%10"ZU"%10"ZU"\n",
					(SIZE_T) x->stats[i].new,
					(SIZE_T) x->stats[i].uniq,
					(SIZE_T) x->stats[i].total,
					(SIZE_T) data[i].len);

		fwrite(x->buf, sizeof(char), strlen(x->buf), x->out);
		x->num_uniq += x->stats[i].new;
	}
	return EXIT_SUCCESS;
}


const int salad_inspect_stub(const config_t* const c, const data_processor_t* const dp, file_t* const f_in, FILE* const f_out)
{
	salad_header("Inspect", &f_in->meta, c);
	SALAD_T(training);

	if (c->bloom != NULL && salad_from_file_v("training", c->bloom, &training) != EXIT_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	// TODO: right now only bloom filters are possible
	const int newBloomFilter = (TO_BLOOMFILTER(training.model) == NULL);
	if (newBloomFilter)
	{
		salad_from_config(&training, c);
	}

	SALAD_T(cur);
	salad_from_config(&cur, c);

	const model_type_t t = to_model_type(cur.as_binary, __(cur).use_tokens);

	inspect_t context = {
			.fct = pick_wrapper(t, newBloomFilter),
			.param = {TO_BLOOMFILTER(training.model), TO_BLOOMFILTER(cur.model), cur.ngram_length, __(cur).delimiter.d},
			.buf = {0},
			.stats = (bloomize_stats_t*) calloc(c->batch_size, sizeof(bloomize_stats_t)),
			.num_uniq = 0,
			.out = f_out
	};

	dp->recv(f_in, salad_inspect_callback, c->batch_size, &context);

	BLOOM* const cur_model = TO_BLOOMFILTER(cur.model);

	const size_t N = bloom_count(cur_model);
	info("Saturation: %.3f%%", (((double)N)/ ((double)cur_model->bitsize))*100);

	const uint8_t k = cur_model->nfuncs;
	const long double n = (long double) context.num_uniq;
	const long double m = (long double) cur_model->bitsize;
	info("Expected error: %.3Lf%%", pow(1 - exp(-(k*n)/ m), k) *100);

	free(context.stats);

	salad_destroy(&training);
	salad_destroy(&cur);
	return EXIT_SUCCESS;
}


const int _salad_inspect_(const config_t* const c)
{
	return salad_heart(c, salad_inspect_stub);
}
