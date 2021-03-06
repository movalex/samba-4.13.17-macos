/*
 * Unix SMB/CIFS implementation.
 *
 * Copyright (C) 2021 Ralph Boehme <slow@samba.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "adouble.c"
#include <cmocka.h>

static int setup_talloc_context(void **state)
{
	TALLOC_CTX *frame = talloc_stackframe();

	*state = frame;
	return 0;
}

static int teardown_talloc_context(void **state)
{
	TALLOC_CTX *frame = *state;

	TALLOC_FREE(frame);
	return 0;
}

/*
 * Basic and sane buffer.
 */
static uint8_t ad_basic[] = {
	0x00, 0x05, 0x16, 0x07, /* Magic */
	0x00, 0x02, 0x00, 0x00, /* Version */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x02,             /* Count */
	/* adentry 1: FinderInfo */
	0x00, 0x00, 0x00, 0x09,	/* eid: FinderInfo */
	0x00, 0x00, 0x00, 0x32,	/* offset */
	0x00, 0x00, 0x00, 0x20,	/* length */
	/* adentry 2: Resourcefork */
	0x00, 0x00, 0x00, 0x02,	/* eid: Resourcefork */
	0x00, 0x00, 0x00, 0x52,	/* offset */
	0xff, 0xff, 0xff, 0x00,	/* length */
	/* FinderInfo data: 32 bytes */
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

/*
 * An empty FinderInfo entry.
 */
static uint8_t ad_finderinfo1[] = {
	0x00, 0x05, 0x16, 0x07, /* Magic */
	0x00, 0x02, 0x00, 0x00, /* Version */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x02,             /* Count */
	/* adentry 1: FinderInfo */
	0x00, 0x00, 0x00, 0x09,	/* eid: FinderInfo */
	0x00, 0x00, 0x00, 0x52,	/* off: points at end of buffer */
	0x00, 0x00, 0x00, 0x00,	/* len: 0, so off+len don't exceed bufferlen */
	/* adentry 2: Resourcefork */
	0x00, 0x00, 0x00, 0x02,	/* eid: Resourcefork */
	0x00, 0x00, 0x00, 0x52,	/* offset */
	0xff, 0xff, 0xff, 0x00,	/* length */
	/* FinderInfo data: 32 bytes */
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

/*
 * A dangerous FinderInfo with correct length exceeding buffer by one byte.
 */
static uint8_t ad_finderinfo2[] = {
	0x00, 0x05, 0x16, 0x07, /* Magic */
	0x00, 0x02, 0x00, 0x00, /* Version */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x02,             /* Count */
	/* adentry 1: FinderInfo */
	0x00, 0x00, 0x00, 0x09,	/* eid: FinderInfo */
	0x00, 0x00, 0x00, 0x33,	/* off: points at beginng of data + 1 */
	0x00, 0x00, 0x00, 0x20,	/* len: 32, so off+len exceeds bufferlen by 1 */
	/* adentry 2: Resourcefork */
	0x00, 0x00, 0x00, 0x02,	/* eid: Resourcefork */
	0x00, 0x00, 0x00, 0x52,	/* offset */
	0xff, 0xff, 0xff, 0x00,	/* length */
	/* FinderInfo data: 32 bytes */
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

static uint8_t ad_finderinfo3[] = {
	0x00, 0x05, 0x16, 0x07, /* Magic */
	0x00, 0x02, 0x00, 0x00, /* Version */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x02,             /* Count */
	/* adentry 1: FinderInfo */
	0x00, 0x00, 0x00, 0x09,	/* eid: FinderInfo */
	0x00, 0x00, 0x00, 0x33,	/* off: points at beginng of data + 1 */
	0x00, 0x00, 0x00, 0x1f,	/* len: 31, so off+len don't exceed buf */
	/* adentry 2: Resourcefork */
	0x00, 0x00, 0x00, 0x02,	/* eid: Resourcefork */
	0x00, 0x00, 0x00, 0x52,	/* offset */
	0xff, 0xff, 0xff, 0x00,	/* length */
	/* FinderInfo data: 32 bytes */
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

/*
 * A dangerous name entry.
 */
static uint8_t ad_name[] = {
	0x00, 0x05, 0x16, 0x07, /* Magic */
	0x00, 0x02, 0x00, 0x00, /* Version */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x02,             /* Count */
	/* adentry 1: FinderInfo */
	0x00, 0x00, 0x00, 0x09,	/* eid: FinderInfo */
	0x00, 0x00, 0x00, 0x32,	/* offset */
	0x00, 0x00, 0x00, 0x20,	/* length */
	/* adentry 2: Name */
	0x00, 0x00, 0x00, 0x03,	/* eid: Name */
	0x00, 0x00, 0x00, 0x52,	/* off: points at end of buffer */
	0x00, 0x00, 0x00, 0x01,	/* len: 1, so off+len exceeds bufferlen */
	/* FinderInfo data: 32 bytes */
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

/*
 * A empty ADEID_FILEDATESI entry.
 */
static uint8_t ad_date1[] = {
	0x00, 0x05, 0x16, 0x07, /* Magic */
	0x00, 0x02, 0x00, 0x00, /* Version */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x02,             /* Count */
	/* adentry 1: FinderInfo */
	0x00, 0x00, 0x00, 0x09,	/* eid: FinderInfo */
	0x00, 0x00, 0x00, 0x32,	/* offset */
	0x00, 0x00, 0x00, 0x20,	/* length */
	/* adentry 2: Dates */
	0x00, 0x00, 0x00, 0x08,	/* eid: dates */
	0x00, 0x00, 0x00, 0x52,	/* off: end of buffer */
	0x00, 0x00, 0x00, 0x00,	/* len: 0, empty entry, valid */
	/* FinderInfo data: 32 bytes */
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

/*
 * A dangerous ADEID_FILEDATESI entry, invalid length.
 */
static uint8_t ad_date2[] = {
	0x00, 0x05, 0x16, 0x07, /* Magic */
	0x00, 0x02, 0x00, 0x00, /* Version */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x00, 0x00, 0x00, /* Filler */
	0x00, 0x02,             /* Count */
	/* adentry 1: FinderInfo */
	0x00, 0x00, 0x00, 0x09,	/* eid: FinderInfo */
	0x00, 0x00, 0x00, 0x32,	/* offset */
	0x00, 0x00, 0x00, 0x20,	/* length */
	/* adentry 2: Dates */
	0x00, 0x00, 0x00, 0x08,	/* eid: dates */
	0x00, 0x00, 0x00, 0x43,	/* off: FinderInfo buf but one byte short */
	0x00, 0x00, 0x00, 0x0f,	/* len: 15, so off+len don't exceed bufferlen */
	/* FinderInfo data: 32 bytes */
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

static struct adouble *parse_adouble(TALLOC_CTX *mem_ctx,
				     uint8_t *adbuf,
				     size_t adsize,
				     off_t filesize)
{
	struct adouble *ad = NULL;
	bool ok;

	ad = talloc_zero(mem_ctx, struct adouble);
	ad->ad_data = talloc_zero_size(ad, adsize);
	assert_non_null(ad);

	memcpy(ad->ad_data, adbuf, adsize);

	ok = ad_unpack(ad, 2, filesize);
	if (!ok) {
		return NULL;
	}

	return ad;
}

static void parse_abouble_basic(void **state)
{
	TALLOC_CTX *frame = *state;
	struct adouble *ad = NULL;
	char *p = NULL;

	ad = parse_adouble(frame, ad_basic, sizeof(ad_basic), 0xffffff52);
	assert_non_null(ad);

	p = ad_get_entry(ad, ADEID_FINDERI);
	assert_non_null(p);

	return;
}

static void parse_abouble_finderinfo1(void **state)
{
	TALLOC_CTX *frame = *state;
	struct adouble *ad = NULL;
	char *p = NULL;

	ad = parse_adouble(frame,
			   ad_finderinfo1,
			   sizeof(ad_finderinfo1),
			   0xffffff52);
	assert_non_null(ad);

	p = ad_get_entry(ad, ADEID_FINDERI);
	assert_null(p);

	return;
}

static void parse_abouble_finderinfo2(void **state)
{
	TALLOC_CTX *frame = *state;
	struct adouble *ad = NULL;

	ad = parse_adouble(frame,
			   ad_finderinfo2,
			   sizeof(ad_finderinfo2),
			   0xffffff52);
	assert_null(ad);

	return;
}

static void parse_abouble_finderinfo3(void **state)
{
	TALLOC_CTX *frame = *state;
	struct adouble *ad = NULL;

	ad = parse_adouble(frame,
			   ad_finderinfo3,
			   sizeof(ad_finderinfo3),
			   0xffffff52);
	assert_null(ad);

	return;
}

static void parse_abouble_name(void **state)
{
	TALLOC_CTX *frame = *state;
	struct adouble *ad = NULL;

	ad = parse_adouble(frame, ad_name, sizeof(ad_name), 0x52);
	assert_null(ad);

	return;
}

static void parse_abouble_date1(void **state)
{
	TALLOC_CTX *frame = *state;
	struct adouble *ad = NULL;
	char *p = NULL;

	ad = parse_adouble(frame, ad_date1, sizeof(ad_date1), 0x52);
	assert_non_null(ad);

	p = ad_get_entry(ad, ADEID_FILEDATESI);
	assert_null(p);

	return;
}

static void parse_abouble_date2(void **state)
{
	TALLOC_CTX *frame = *state;
	struct adouble *ad = NULL;

	ad = parse_adouble(frame, ad_date2, sizeof(ad_date2), 0x52);
	assert_null(ad);

	return;
}

int main(int argc, char *argv[])
{
	int rc;
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(parse_abouble_basic),
		cmocka_unit_test(parse_abouble_finderinfo1),
		cmocka_unit_test(parse_abouble_finderinfo2),
		cmocka_unit_test(parse_abouble_finderinfo3),
		cmocka_unit_test(parse_abouble_name),
		cmocka_unit_test(parse_abouble_date1),
		cmocka_unit_test(parse_abouble_date2),
	};

	if (argc == 2) {
		cmocka_set_test_filter(argv[1]);
	}
	cmocka_set_message_output(CM_OUTPUT_SUBUNIT);

	rc = cmocka_run_group_tests(tests,
				    setup_talloc_context,
				    teardown_talloc_context);

	return rc;
}
