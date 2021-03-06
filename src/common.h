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

/**
 * @file
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <util/config.h>

#include <salad/salad.h>
#include <salad/io.h>
#include <salad/container/common.h>
#include <util/config.h>
#include <util/util.h>
#include <util/io.h>

#include <config.h>

#ifndef USE_ARCHIVES
// Just to make sure ;)
#undef GROUPED_INPUT
#endif

typedef enum {
	UNDEFINED,
	TRAINING,
	PREDICT,
	INSPECT,
	STATS,
	TEST
} saladmode_t;

const char* const saladmode_to_string(saladmode_t m);
const saladmode_t to_saladmode(const char* const str);


typedef enum {
	SALAD_EXIT = -1,
	SALAD_RUN = 0,
	SALAD_HELP,
	SALAD_HELP_TRAIN,
	SALAD_HELP_PREDICT,
	SALAD_HELP_INSPECT,
	SALAD_HELP_STATS,
	SALAD_HELP_TEST,
	SALAD_VERSION
} saladstate_t;

typedef struct
{
	saladmode_t mode;
	iomode_t input_type;
	size_t batch_size;
	int group_input;
	char* input_filter;
	char* pcap_filter;
	int net_clientcomm;
	int net_servercomm;
	char* input;
	char* bloom;
	char* bbloom;
	int update_model;
	int transfer_spec; // Transfer the spec of the model to be updated.
	char* output;
	salad_outputfmt_t output_type;
	size_t ngram_length;
	char* delimiter;
	int binary_ngrams;
	int count;
	unsigned int filter_size;
	hashset_t hash_set;
	char* nan;
	int echo_params;
} config_t;

static const config_t DEFAULT_CONFIG =
{
	.mode = UNDEFINED,
	.input_type = IOMODE_LINES,
	.batch_size = 128,
	.group_input = FALSE,
	.input_filter = "",
	.pcap_filter = "tcp",
	.net_clientcomm = TRUE,
	.net_servercomm = TRUE,
	.input = NULL,
	.bloom = NULL,
	.bbloom = NULL,
	.update_model = FALSE,
	.transfer_spec = FALSE,
	.output = NULL,
	.output_type = DEFAULT_OUTPUTFMT,
	.ngram_length = 3,
	.delimiter = NULL,
	.binary_ngrams = FALSE,
	.count = 0,
	.filter_size = 24,
	.hash_set = DEFAULT_HASHSET,
	.nan = "nan",
	.echo_params = FALSE
};


typedef const int (*FN_SALAD)(const config_t* const c, const data_processor_t* const dp, file_t* const f_in, FILE* const f_out);
const int salad_heart(const config_t* const c, FN_SALAD fct);

void salad_header(const char* const msg, const metadata_t* const meta, const config_t* c);
void echo_options(config_t* const config);

const int salad_from_config(salad_t* const s, const config_t* const c);
const int salad_from_file_v(const char* const id, const char* const filename, salad_t* const out);


#endif /* COMMON_H_ */
