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

#include "common.h"
#include "ctest.h"

#include <string.h>


CTEST(util, inline_decode)
{
	char str0[] = "%41%2542%43%20";
	char str1[] = "%41%2542%43%20%";
	char str2[] = "%41%2542%43%20%0";
	char str3[] = "%41%2542%43%20%00";
	char str4[] = "%41%2542%43%20%0x";
	char str5[] = "%41%2542%43%20%x0";
	char str6[] = "%41%2542%43%20%x0%44";
	char str7[] = "¼ pounder with cheese";

	char* strs[] = {
		str0, "A%42C ",
		str1, "A%42C %",
		str2, "A%42C %0",
		str3, "A%42C ",
		str4, "A%42C %0x",
		str5, "A%42C %x0",
		str6, "A%42C %x0D",
		str7, "¼ pounder with cheese", NULL
	};

	for (int i = 0; strs[i] != NULL; i += 2)
	{
		inline_decode(strs[i], strlen(strs[i]));
		ASSERT_STR(strs[i +1], strs[i]);
	}
}

CTEST(util, memcmp_bytes)
{
	static const size_t size = 100;
	uint8_t a[size], b[size], c[size];

	memset(&a, 0x00, sizeof(uint8_t) *size);
	memset(&b, 0x00, sizeof(uint8_t) *size);
	memset(&c, 0xFF, sizeof(uint8_t) *size);


	ASSERT_EQUAL(0, memcmp_bytes(&a, &b, sizeof(uint8_t) *size));
	ASSERT_NOT_EQUAL(0, memcmp_bytes(&a, &c, sizeof(uint8_t) *size));


	b[size -1] = 1;
	ASSERT_EQUAL(-1, memcmp_bytes(&a, &b, sizeof(uint8_t) *size));

	a[size -1] = 2;
	ASSERT_EQUAL(1, memcmp_bytes(&a, &b, sizeof(uint8_t) *size));


	a[42] = 0xFF;
	ASSERT_EQUAL(0xFF, memcmp_bytes(&a, &b, sizeof(uint8_t) *size));

	b[23] = 0xFF;
	ASSERT_EQUAL(-0xFF, memcmp_bytes(&a, &b, sizeof(uint8_t) *size));


	a[0] = 1;
	ASSERT_EQUAL(1, memcmp_bytes(&a, &b, sizeof(uint8_t) *size));

	b[0] = 2;
	ASSERT_EQUAL(-1, memcmp_bytes(&a, &b, sizeof(uint8_t) *size));


	memset(&a, 0xFF, sizeof(uint8_t) *(size -2));
	ASSERT_EQUAL(-0xFF, memcmp_bytes(&a, &c, sizeof(uint8_t) *(size -1)));

	a[size -2] = 0xFF;
	ASSERT_EQUAL(0, memcmp_bytes(&a, &c, sizeof(uint8_t) *(size -1)));

	a[size -1] = 0xFF;
	ASSERT_EQUAL(0, memcmp_bytes(&a, &c, sizeof(uint8_t) *size));
}
