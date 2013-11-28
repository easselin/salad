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

#ifndef TESTS_COMMON_H_
#define TESTS_COMMON_H_

#include "ctest.h"

#include "../salad.h"
#include "../util/util.h"


static const uint8_t NUM_FUNCS = 3;
#define FUNCS sax_hash_n, sdbm_hash_n, bernstein_hash_n

static const char* const DELIMITER = "";
static const char* const TOKEN_DELIMITER = "\r\n \t";
static const size_t NGRAM_LENGTH = 3;

static const char* const TEST_STR = "The quick brown fox jumps over the lazy dog";

#endif /* TESTS_COMMON_H_ */