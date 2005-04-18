/*
 *      PROGRAM:        JRD Access Method
 *      MODULE:         Move.cpp
 *      DESCRIPTION:    Data mover and converter and comparator, etc.
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete ports:
 *                          - DELTA and IMP
 *
 * 2001.6.16 Claudio Valderrama: Wiped out the leading space in
 * cast(float_expr as char(n)) in dialect 1, reported in SF.
 * 2001.11.19 Claudio Valderrama: integer_to_text() should use the
 * source descriptor "from" to call conversion_error.
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 *
 * 2003.2.10 Jim Starkey -- Created from cvt.cpp
 *
 */
 
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "firebird.h"
#include "common.h"
#include "Move.h"
//#include "quad.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef DBL_MAX_10_EXP
#undef DBL_MAX_10_EXP
#define DBL_MAX_10_EXP          308
#endif

#ifdef SCO_UNIX
#define DBL_MAX_10_EXP          308
#endif

#ifdef MVS  // define so float.h not included from MVA MVS/h
#ifdef __BFP__
#define DBL_MAX_10_EXP          308
#else
#define DBL_MAX_10_EXP          75
#endif
#endif

#ifndef DBL_MAX_10_EXP
#include <float.h>
#endif

#ifndef VMS
#define DEFAULT_DOUBLE	dtype_double
#else

#define DEFAULT_DOUBLE	dtype_double
#define SPECIAL_DOUBLE	dtype_d_float
#define CNVT_TO_DFLT(x)	MTH$CVT_D_G (x)
#define CNVT_FROM_DFLT(x)	MTH$CVT_G_D (x)
double MTH$CVT_D_G(), MTH$CVT_G_D();

#endif


#ifndef fb_assert
#define fb_assert(conditiona)
#endif


/* normally the following two definitions are part of limits.h
   but due to a compiler bug on Apollo casting LONG_MIN to be a
   double, these have to be defined as double numbers...

   Also, since SunOS4.0 doesn't want to play with the rest of
   the ANSI world, these definitions have to be included explicitly.
   So, instead of some including <limits.h> and others using these
   definitions, just always use these definitions (huh?) */

#define LONG_MIN_real   -2147483648.	/* min decimal value of an "SLONG" */
#define LONG_MAX_real   2147483647.	/* max decimal value of an "SLONG" */
#define LONG_MIN_int    -2147483648	/* min integer value of an "SLONG" */
#define LONG_MAX_int    2147483647	/* max integer value of an "SLONG" */

/* It turns out to be tricky to write the INT64 versions of those constant in
   a way that will do the right thing on all platforms.  Here we go. */

#define LONG_MAX_int64 ((SINT64)2147483647)	/* max int64 value of an SLONG */
#define LONG_MIN_int64 (-LONG_MAX_int64-1)	/* min int64 value of an SLONG */

#define QUAD_MIN_real   -9223372036854775808.	/* min decimal value of quad */
#define QUAD_MAX_real   9223372036854775807.	/* max decimal value of quad */

#define QUAD_MIN_int    quad_min_int	/* min integer value of quad */
#define QUAD_MAX_int    quad_max_int	/* max integer value of quad */

#define FLOAT_MAX       FLT_MAX			/* 3.4e38 max float (32 bit) value  */

#ifdef NATIVE_QUAD
static const SQUAD quad_min_int = LONG_MIN;
static const SQUAD quad_max_int = LONG_MAX;

#else

#ifndef WORDS_BIGENDIAN
static const SQUAD quad_min_int = { 0, LONG_MIN };
static const SQUAD quad_max_int = { -1, LONG_MAX };
#else
static const SQUAD quad_min_int = { LONG_MIN, 0 };
static const SQUAD quad_max_int = { LONG_MAX, -1 };
#endif
#endif

#ifndef WORDS_BIGENDIAN
#define LOW_WORD        0
#define HIGH_WORD       1
#else
#define LOW_WORD        1
#define HIGH_WORD       0
#endif

#define LETTER7(c)      ((c) >= 'A' && (c) <= 'Z')
#define DIGIT(c)        ((c) >= '0' && (c) <= '9')
#define ABSOLUT(x)      ((x) < 0 ? -(x) : (x))

/* The expressions for SHORT_LIMIT, LONG_LIMIT, INT64_LIMIT and
 * QUAD_LIMIT return the largest value that permit you to multiply by
 * 10 without getting an overflow.  The right operand of the << is two
 * less than the number of bits in the type: one bit is for the sign,
 * and the other is because we divide by 5, rather than 10.  */

#define SHORT_LIMIT     ((1 << 14) / 5)
#define LONG_LIMIT      ((1L << 30) / 5)

#define COMMA           ','

/* NOTE: The syntax for the below line may need modification to ensure
 *	 the result of 1 << 62 is a quad
 */
#define QUAD_LIMIT      ((((SINT64) 1) << 62) / 5)
#define INT64_LIMIT     ((((SINT64) 1) << 62) / 5)
#define NUMERIC_LIMIT   (INT64_LIMIT)

#define TODAY           "TODAY"
#define NOW             "NOW"
#define TOMORROW        "TOMORROW"
#define YESTERDAY       "YESTERDAY"

#define CVT_FAILURE_SPACE       128

#define CVT_COPY_BUFF(from,to,len) \
{if (len) {MOVE_FAST(from,to,len); from +=len; to+=len; len=0;} }

static const TEXT* const months[] = {
	"JANUARY",
	"FEBRUARY",
	"MARCH",
	"APRIL",
	"MAY",
	"JUNE",
	"JULY",
	"AUGUST",
	"SEPTEMBER",
	"OCTOBER",
	"NOVEMBER",
	"DECEMBER",
	NULL
};

static const int dtypeLengths [DTYPE_TYPE_MAX] =
{
	0,							/* dtype_unknown */
	0,							/* dtype_text */
	0,							/* dtype_cstring */
	0,							/* dtype_varying */
	0,
	0,
	0,							/* dtype_packed */
	0,							/* dtype_byte */
	6,							/* dtype_short      -32768 */
	11,							/* dtype_long       -2147483648 */
	20,							/* dtype_quad       -9223372036854775808 */
	15,							/* dtype_real       -1.23456789e+12 */
	24,							/* dtype_double     -1.2345678901234567e+123 */
	24,							/* dtype_d_float (believed to have this range)  -1.2345678901234567e+123 */
	10,							/* dtype_sql_date   YYYY-MM-DD */
	13,							/* dtype_sql_time   HH:MM:SS.MMMM */
	25,							/* dtype_timestamp  YYYY-MM-DD HH:MM:SS.MMMM */
	/*  -- in BLR_version4  DD-Mon-YYYY HH:MM:SS.MMMM */
	9,							/* dtype_blob       FFFF:FFFF */
	9,							/* dtype_array      FFFF:FFFF */
	21							/* dtype_int64      -9223372036854775808 + decimal point */
};
	

double Move::dateToDouble(const dsc *desc)
{
	SLONG temp[2], *date;

	/* If the input descriptor is not in date form, convert it. */

	switch (desc->dsc_dtype)
		{
		case dtype_timestamp:
			date = (SLONG*) desc->dsc_address;
			break;
		
		case dtype_sql_time:
			/* Temporarily convert the time to a timestamp for conversion */
			date = temp;
			date[0] = 0;
			date[1] = *(SLONG*) desc->dsc_address;
			break;
		
		case  dtype_sql_date:
			/* Temporarily convert the date to a timestamp for conversion */
			date = temp;
			date[0] = *(SLONG*) desc->dsc_address;
			date[1] = 0;
			break;
		
		default:
			/* Unknown type - most likely a string.  Try to convert it to a 
			timestamp -- or die trying (reporting an error).
			Don't worry about users putting a TIME or DATE here - this
			conversion is occuring in really flexible spots - we don't
			want to overdo it. */

			dsc temp_desc;
			MOVE_CLEAR(&temp_desc, sizeof(temp_desc));

			temp_desc.dsc_dtype = dtype_timestamp;
			temp_desc.dsc_length = sizeof(temp);
			date = temp;
			temp_desc.dsc_address = (UCHAR*) date;
			move(desc, &temp_desc);
		}

	return date [0] + (double) date[1] / (24. * 60. * 60. * ISC_TIME_SECONDS_PRECISION);
}

void Move::conversionError(const dsc* source)
{
}

void Move::move(const dsc* from, const dsc* to)
{
	//int fill;
	SLONG l;
	//USHORT strtype;

	SLONG length = from->dsc_length;
	char* p = (char*) to->dsc_address;
	const char* q = (char*) from->dsc_address;

	/* If the datatypes and lengths are identical, just move the
	stuff byte by byte.  Although this may seem slower than 
	optimal, it would cost more to find the fast move than the
	fast move would gain. */

	if (DSC_EQUIV(from, to))
		{
		memcpy (p, q, length);
		return;
		}

	/* Do data type by data type conversions.  Not all are supported,
	and some will drop out for additional handling. */

	switch (to->dsc_dtype) 
		{
		case dtype_timestamp:
			switch (from->dsc_dtype) 
				{
				case dtype_varying:
				case dtype_cstring:
				case dtype_text:
					{
					GDS_TIMESTAMP date;
					tm times;

					//string_to_datetime(from, &date, expect_timestamp, err);
					date = stringToDatetime (from, expect_timestamp);

					decodeTimestamp(&date, &times);
					if ((times.tm_year + 1900) < MIN_YEAR || (times.tm_year) + 1900 > MAX_YEAR)
						dateRangeExceeded();

					((GDS_TIMESTAMP *) to->dsc_address)->timestamp_date = date.timestamp_date;
					((GDS_TIMESTAMP *) to->dsc_address)->timestamp_time = date.timestamp_time;
					}
					return;

				case dtype_sql_date:
					((GDS_TIMESTAMP *) (to->dsc_address))->timestamp_date =
						*(GDS_DATE *) (from->dsc_address);
					((GDS_TIMESTAMP *) (to->dsc_address))->timestamp_time = 0;
					return;

				case dtype_sql_time:
					((GDS_TIMESTAMP*) (to->dsc_address))->timestamp_date = 0;
					((GDS_TIMESTAMP*) (to->dsc_address))->timestamp_time = *(GDS_TIME*) (from->dsc_address);

					/* Per SQL Specs, we need to set the DATE
					portion to the current date */
					{
					time_t clock = getCurrentTime();
					const tm times = *localtime(&clock);
					GDS_TIMESTAMP enc_times = encodeTimestamp (&times);
					//isc_encode_timestamp(&times, &enc_times);
					((GDS_TIMESTAMP*) (to->dsc_address))->timestamp_date =
						enc_times.timestamp_date;
					}
					return;

				case dtype_short:
				case dtype_long:
				case dtype_int64:
				case dtype_quad:
				case dtype_real:
				case dtype_double:
		#ifdef VMS
				case dtype_d_float:
		#endif
					conversionError(from);				// never coming back
					break;
					
				default:
					notImplemented();					// never coming back
				}
			break;

		case dtype_sql_date:
			switch (from->dsc_dtype) 
				{
				case dtype_varying:
				case dtype_cstring:
				case dtype_text:
					{
					//string_to_datetime(from, &date, expect_sql_date, err);
					GDS_TIMESTAMP date = stringToDatetime (from, expect_sql_date);
					tm times;
					//isc_decode_timestamp(&date, &times);
					decodeTimestamp (&date, &times);
					if ((times.tm_year + 1900) < MIN_YEAR || (times.tm_year) + 1900 > MAX_YEAR)
						dateRangeExceeded();
					*((GDS_DATE *) to->dsc_address) = date.timestamp_date;
					}
					return;

				case dtype_timestamp:
					{
					GDS_TIMESTAMP new_date;
					new_date.timestamp_date = ((GDS_TIMESTAMP *) from->dsc_address)->timestamp_date;
					new_date.timestamp_time = 0;
					tm times;
					//isc_decode_timestamp(&new_date, &times);
					decodeTimestamp (&new_date, &times);
					if ((times.tm_year + 1900) < MIN_YEAR || (times.tm_year) + 1900 > MAX_YEAR)
						dateRangeExceeded();
					*((GDS_DATE *) to->dsc_address) =
						((GDS_TIMESTAMP *) from->dsc_address)->timestamp_date;
					}
					return;

				case dtype_sql_time:
				case dtype_short:
				case dtype_long:
				case dtype_int64:
				case dtype_quad:
				case dtype_real:
				case dtype_double:
		#ifdef VMS
				case dtype_d_float:
		#endif
					conversionError(from);
					break;
				
				default:
					notImplemented();
				}
			break;

		case dtype_sql_time:
			switch (from->dsc_dtype) 
				{
				case dtype_varying:
				case dtype_cstring:
				case dtype_text:
					{
					GDS_TIMESTAMP date = stringToDatetime (from, expect_sql_time);
					*(GDS_TIME *) to->dsc_address = date.timestamp_time;
					}
					return;

				case dtype_timestamp:
					*((GDS_TIME *) to->dsc_address) =
						((GDS_TIMESTAMP *) from->dsc_address)->timestamp_time;
					return;

				case dtype_sql_date:
				case dtype_short:
				case dtype_long:
				case dtype_int64:
				case dtype_quad:
				case dtype_real:
				case dtype_double:
#ifdef VMS
				case dtype_d_float:
#endif
					conversionError(from);
					break;
					
				default:
					notImplemented();
				}
			break;

		case dtype_text:
		case dtype_cstring:
		case dtype_varying:
			moveToString (from, to);
			return;

		case dtype_blob:
		case dtype_array:
			if (from->dsc_dtype == dtype_quad) 
				{
				((SLONG *) p)[0] = ((SLONG *) q)[0];
				((SLONG *) p)[1] = ((SLONG *) q)[1];
				return;
				}

			if (to->dsc_dtype != from->dsc_dtype)
				wishList();				// not coming back...

			/* Note: DSC_EQUIV failed above as the blob sub_types were different,
			* or their character sets were different.  In V4 we aren't trying
			* to provide blob type integrity, so we just assign the blob id
			*/

			/* Move blob_id byte-by-byte as that's the way it was done before */
			CVT_COPY_BUFF(q, p, length);
			return;

		case dtype_short:
			l = getLong (from, to->dsc_scale);
			/* TMN: Here we should really have the following fb_assert */
			/* fb_assert(l <= MAX_SSHORT); */
			*(SSHORT*) p = (SSHORT) l;
			if (*(SSHORT *) p != l)
				arithmeticException();
			return;

		case dtype_long:
			*(SLONG*) p = getLong (from, to->dsc_scale);
			return;

		case dtype_int64:
			//*(SINT64*) p = CVT_get_int64(from, (SSHORT) to->dsc_scale, err);
			*(SINT64*) p = getInt64 (from, to->dsc_scale);
			return;

		case dtype_quad:
			if (from->dsc_dtype == dtype_blob || from->dsc_dtype == dtype_array) 
				{
				((SLONG *) p)[0] = ((SLONG *) q)[0];
				((SLONG *) p)[1] = ((SLONG *) q)[1];
				return;
				}
			//*(SQUAD *) p = CVT_get_quad(from, (SSHORT) to->dsc_scale, err);
			*(SQUAD *) p = getQuad (from, to->dsc_scale);
			return;

		case dtype_real:
			{
			//double d_value = CVT_get_double(from, err);
			double d_value = getDouble(from);
			if (ABSOLUT(d_value) > FLOAT_MAX)
				arithmeticException();
			*(float*) p = (float) d_value;
			}
			return;

		case dtype_double:
			*(double*) p = getDouble(from);
			return;

#ifdef VMS
		case SPECIAL_DOUBLE:
			*(double*) p = getDouble(from);
			*(double*) p = CNVT_FROM_DFLT((double*) p);
			return;
#endif
		}

	if (from->dsc_dtype == dtype_array || from->dsc_dtype == dtype_blob)
		wishList();			// not coming back

	notImplemented();	// not coming back
}

/*
 *      Convert an arbitrary string to a date and/or time.
 *
 *      String must be formed using ASCII characters only.
 *      Conversion routine can handle the following input formats
 *      "now"           current date & time
 *      "today"         Today's date       0:0:0.0 time
 *      "tomorrow"      Tomorrow's date    0:0:0.0 time
 *      "yesterday"     Yesterday's date   0:0:0.0 time
 *      YYYY-MM-DD [HH:[Min:[SS.[Thou]]]]] 
 *      MM:DD[:YY [HH:[Min:[SS.[Thou]]]]] 
 *      DD.MM[:YY [HH:[Min:[SS.[Thou]]]]] 
 *      Where:
 *        DD = 1  .. 31    (Day of month)
 *        YY = 00 .. 99    2-digit years are converted to the nearest year
 *		           in a 50 year range.  Eg: if this is 1996
 *                              96 ==> 1996
 *                              97 ==> 1997
 *                              ...
 *                              00 ==> 2000
 *                              01 ==> 2001
 *                              ...
 *                              44 ==> 2044
 *                              45 ==> 2045
 *                              46 ==> 1946
 *                              47 ==> 1947
 *                              ...
 *                              95 ==> 1995
 *                         If the current year is 1997, then 46 is converted
 *                         to 2046 (etc).
 *           = 100.. 5200  (Year) 
 *        MM = 1  .. 12    (Month of year)
 *           = "JANUARY"... (etc)
 *        HH = 0  .. 23    (Hour of day)
 *       Min = 0  .. 59    (Minute of hour)
 *        SS = 0  .. 59    (Second of minute - LEAP second not supported)
 *      Thou = 0  .. 9999  (Fraction of second)
 *      HH, Min, SS, Thou default to 0 if missing.
 *      YY defaults to current year if missing.
 *      Note: ANY punctuation can be used instead of : (eg: / - etc)
 *            Using . (period) in either of the first two separation
 *            points will cause the date to be parsed in European DMY
 *            format.
 *            Arbitrary whitespace (space or TAB) can occur between
 *            components.
 *
 *		Was string_to_datetime();
 */

#define		ENGLISH_MONTH	-1
#define		SPECIAL		-2

GDS_TIMESTAMP Move::stringToDatetime(const dsc* desc, ExpectDateTime expect_type)
{
	/* Values inside of description
		> 0 is number of digits 
		0 means missing 
		ENGLISH_MONTH for the presence of an English month name
		SPECIAL       for a special date verb */
		
	USHORT position_year = 0;
	USHORT position_month = 1;
	USHORT position_day = 2;
	bool have_english_month = false;
	bool dot_separator_seen = false;
	time_t clock;
	tm times, times2;
	TEXT buffer[100];			/* arbitrarily large */

	const char* string;
	const USHORT length = makeString (desc, ttype_ascii, &string, (vary*) buffer, sizeof (buffer));
		/***
		CVT_make_string(desc, ttype_ascii, &string,
						(VARY *) buffer, sizeof(buffer), err);
		***/			
	const char* p = string;
	const char* const end = p + length;

	USHORT n, components[7];
	SSHORT description[7];
	memset(components, 0, sizeof(components));
	memset(description, 0, sizeof(description));

/* Parse components */
/* The 7 components are Year, Month, Day, Hours, Minutes, Seconds, Thou */
/* The first 3 can be in any order */

	const int start_component = (expect_type == expect_sql_time) ? 3 : 0;
	int i;
	
	for (i = start_component; i < 7; i++) 
		{

		/* Skip leading blanks.  If we run out of characters, we're done
		   with parse.  */

		while (p < end && (*p == ' ' || *p == '\t'))
			p++;
		if (p == end)
			break;

		/* Handle digit or character strings */

		TEXT c = UPPER7(*p);
		
		if (DIGIT(c)) 
			{
			USHORT precision = 0;
			n = 0;
			while (p < end && DIGIT(*p)) 
				{
				n = n * 10 + *p++ - '0';
				precision++;
				}
			description[i] = precision;
			}
		else if (LETTER7(c) && !have_english_month) 
			{
			TEXT temp[sizeof(YESTERDAY) + 1];
			TEXT* t = temp;
			
			while ((p < end) && (t < &temp[sizeof(temp) - 1])) 
				{
				c = UPPER7(*p);
				if (!LETTER7(c))
					break;
				*t++ = c;
				p++;
				}
				
			*t = 0;

			/* Insist on at least 3 characters for month names */
			
			if (t - temp < 3) 
				conversionError(desc);

			const TEXT* const* month_ptr = months;
			
			while (true) 
				{
				/* Month names are only allowed in first 2 positions */
				if (*month_ptr && i < 2) 
					{
					t = temp;
					const TEXT* m = *month_ptr++;
					while (*t && *t == *m) 
						{
						++t;
						++m;
						}
					if (!*t)
						break;
					}
				else 
					{
					/* it's not a month name, so it's either a magic word or
					   a non-date string.  If there are more characters, it's bad */

					description[i] = SPECIAL;

					while (++p < end)
						if (*p != ' ' && *p != '\t' && *p != 0)
							conversionError(desc);

					/* fetch the current time */

					clock = time(0);
					times2 = *localtime(&clock);

					if (strcmp(temp, NOW) == 0) 
						{
						//isc_encode_timestamp(&times2, date);
						//return;
						return encodeTimestamp (&times2);
						}
						
					if (expect_type == expect_sql_time) 
						conversionError(desc);
						
					times2.tm_hour = times2.tm_min = times2.tm_sec = 0;
					GDS_TIMESTAMP date = encodeTimestamp (&times2);
					//isc_encode_timestamp(&times2, date);
					
					if (strcmp(temp, TODAY) == 0)
						return date;
						
					if (strcmp(temp, TOMORROW) == 0) 
						{
						date.timestamp_date++;
						return date;
						}
						
					if (strcmp(temp, YESTERDAY) == 0) 
						{
						date.timestamp_date--;
						return date;
						}
						
					conversionError(desc);
					}
				}
				
			n = month_ptr - months;
			position_month = i;
			description[i] = ENGLISH_MONTH;
			have_english_month = true;
			}
		else 
			conversionError(desc); ///* Not a digit and not a letter - must be punctuation */

		components[i] = n;

		/* Grab whitespace following the number */
		while (p < end && (*p == ' ' || *p == '\t'))
			p++;

		if (p == end)
			break;

		/* Grab a separator character */
		if (*p == '/' || *p == '-' || *p == ',' || *p == ':') 
			{
			p++;
			continue;
			}
			
		if (*p == '.') 
			{
			if (i <= 1)
				dot_separator_seen = true;
			p++;
			continue;
			}
		}

	/* User must provide at least 2 components */
	
	if (i - start_component < 1) 
		conversionError(desc);

	/* Dates cannot have a Time portion */
	
	if (expect_type == expect_sql_date && i > 2)
		conversionError(desc);

	memset(&times, 0, sizeof(times));

	if (expect_type != expect_sql_time) 
		{
		/* Figure out what format the user typed the date in */

		/* A 4 digit number to start implies YYYY-MM-DD */
		
		if (description[0] >= 3) 
			{
			position_year = 0;
			position_month = 1;
			position_day = 2;
			}

		/* An English month to start implies MM-DD-YYYY */
		
		else if (description[0] == ENGLISH_MONTH) 
			{
			position_year = 2;
			position_month = 0;
			position_day = 1;
			}

		/* An English month in the middle implies DD-MM-YYYY */
		else if (description[1] == ENGLISH_MONTH) 
			{
			position_year = 2;
			position_month = 1;
			position_day = 0;
			}

		/* A period as a separator implies DD.MM.YYYY */
		else if (dot_separator_seen) 
			{
			position_year = 2;
			position_month = 1;
			position_day = 0;
			}

		/* Otherwise assume MM-DD-YYYY */
		else 
			{
			position_year = 2;
			position_month = 0;
			position_day = 1;
			}

		/* Forbid years with more than 4 digits */
		/* Forbid months or days with more than 2 digits */
		/* Forbid months or days being missing */
		
		if (description[position_year] > 4 ||
			description[position_month] > 2
			|| description[position_month] == 0
			|| description[position_day] > 2
			|| description[position_day] <= 0)
			conversionError(desc);

		/* Slide things into day, month, year form */

		times.tm_year = components[position_year];
		times.tm_mon = components[position_month];
		times.tm_mday = components[position_day];

		/* Handle defaulting of year */

		if (description[position_year] == 0) 
			{
			clock = time(0);
			times2 = *localtime(&clock);
			times.tm_year = times2.tm_year + 1900;
			}

		/* Handle conversion of 2-digit years */

		else if (description[position_year] <= 2) 
			{
			clock = time(0);
			times2 = *localtime(&clock);
			if (times.tm_year < (times2.tm_year - 50) % 100)
				times.tm_year += 2000;
			else
				times.tm_year += 1900;
			}

		times.tm_year -= 1900;
		times.tm_mon -= 1;
		}
	else 
		{
		/* The date portion isn't needed for time - but to
		   keep the conversion in/out of isc_time clean lets
		   intialize it properly anyway */
		times.tm_year = 0;
		times.tm_mon = 0;
		times.tm_mday = 1;
		}

	/* Handle time values out of range - note possibility of 60 for
	* seconds value due to LEAP second (Not supported in V4.0).
	*/
	
	if (((times.tm_hour = components[3]) > 23) ||
		((times.tm_min = components[4]) > 59) ||
		((times.tm_sec = components[5]) > 59) ||
		(description[6] > -ISC_TIME_SECONDS_PRECISION_SCALE))
		conversionError(desc);

	/* convert day/month/year to Julian and validate result
	This catches things like 29-Feb-1995 (not a leap year) */

	//isc_encode_timestamp(&times, date);
	GDS_TIMESTAMP date = encodeTimestamp (&times2);
	
	if (expect_type != expect_sql_time) 
		{
		//isc_decode_timestamp(date, &times2);
		decodeTimestamp (&date, &times2);

		if ((times.tm_year + 1900) < MIN_YEAR || (times.tm_year) + 1900 > MAX_YEAR)
			dateRangeExceeded();

		if (times.tm_year != times2.tm_year ||
			times.tm_mon != times2.tm_mon ||
			times.tm_mday != times2.tm_mday ||
			times.tm_hour != times2.tm_hour ||
			times.tm_min != times2.tm_min || times.tm_sec != times2.tm_sec)
			conversionError(desc);
		};

/* Convert fraction of seconds */
	while (description[6]++ < -ISC_TIME_SECONDS_PRECISION_SCALE)
		components[6] *= 10;

	date.timestamp_time += components[6];
	
	return date;
}

void Move::decodeTimestamp(GDS_TIMESTAMP* date, struct tm *times)
{
	memset(times, 0, sizeof(*times));
	decodeDate(date->timestamp_date, times);
	times->tm_yday = getDayOfYear(times);
	
	if ((times->tm_wday = (date->timestamp_date + 3) % 7) < 0)
		times->tm_wday += 7;

	const ULONG minutes = date->timestamp_time / (ISC_TIME_SECONDS_PRECISION * 60);
	times->tm_hour = minutes / 60;
	times->tm_min = minutes % 60;
	times->tm_sec = (date->timestamp_time / ISC_TIME_SECONDS_PRECISION) % 60;
}

void Move::dateRangeExceeded(void)
{
	//(*err) (isc_date_range_exceeded, 0);
}

time_t Move::getCurrentTime(void)
{
#ifdef CRUD
	TDBB tdbb = NULL;
	time_t clock;

	/** Cannot call GET_THREAD_DATA because that macro calls 
		BUGCHECK i.e. ERR_bugcheck() which is not part of 
		client library **/
		
	tdbb = PLATFORM_GET_THREAD_DATA;

	/* If we're in the engine, then the THDD type must
	be a THDD_TYPE_TDBB.  So, if we're in the engine
	and have a request, pull the effective date out
	of the request timestamp.
	Otherwise, take the CURRENT date to populate the 
	date portion of the timestamp */

	if ((tdbb) &&
		(((THDD) tdbb)->thdd_type == THDD_TYPE_TDBB) &&
		tdbb->tdbb_request) 
		{
		if (tdbb->tdbb_request->req_timestamp)
			clock = tdbb->tdbb_request->req_timestamp;
		else 
			{
			/* All requests should have a timestamp */
			fb_assert(FALSE);
			clock = time(0);
			}
		}
	else
		clock = time(0);
#endif

	return time(0);
}

GDS_TIMESTAMP Move::encodeTimestamp(const struct tm* times)
{
	GDS_TIMESTAMP date;
	
	date.timestamp_date = getGDSDate(times);
	date.timestamp_time =
		((times->tm_hour * 60 + times->tm_min) * 60 +
		 times->tm_sec) * ISC_TIME_SECONDS_PRECISION;
	
	return date;
}

void Move::notImplemented(void)
{
}

CHARSET_ID Move::getCharacterSet(const dsc* desc)
{
	/***
	if ((INTL_TTYPE(from) == ttype_dynamic) && (err == ERR_post))
		return INTL_charset(NULL, INTL_TTYPE(from), err);
	***/
	
	return INTL_TTYPE(desc);
}

void Move::moveInternational(const dsc* from, const dsc* to)
{
	notImplemented();
}

int Move::getStringPointer(const dsc* desc, USHORT* ttype, char** address, vary* temp, int length)
{
	/***
	fb_assert(desc != NULL);
	fb_assert(ttype != NULL);
	fb_assert(address != NULL);
	fb_assert(err != NULL);
	fb_assert((((temp != NULL) && (length > 0))
			|| (desc->dsc_dtype == dtype_text)
			|| (desc->dsc_dtype == dtype_cstring)
			|| (desc->dsc_dtype == dtype_varying)));
	***/
	
	/* If the value is already a string (fixed or varying), just return
	the address and length. */

	if (desc->dsc_dtype <= dtype_any_text) 
		{
		*address = (char*) desc->dsc_address;
		*ttype = INTL_TTYPE(desc);
		
		if (desc->dsc_dtype == dtype_text)
			return desc->dsc_length;
			
		if (desc->dsc_dtype == dtype_cstring)
			return MIN((int) strlen((char*) desc->dsc_address), desc->dsc_length - 1);
			
		if (desc->dsc_dtype == dtype_varying) 
			{
			vary* varying = (vary*) desc->dsc_address;
			*address = varying->vary_string;
			return MIN(varying->vary_length, (int) (desc->dsc_length - sizeof(USHORT)));
			}
		}

/* No luck -- convert value to varying string. */

	dsc temp_desc;
	MOVE_CLEAR(&temp_desc, sizeof(temp_desc));
	temp_desc.dsc_length = length;
	temp_desc.dsc_address = (UCHAR *) temp;
	INTL_ASSIGN_TTYPE(&temp_desc, ttype_ascii);
	temp_desc.dsc_dtype = dtype_varying;
	move(desc, &temp_desc);
	*address = temp->vary_string;
	*ttype = INTL_TTYPE(&temp_desc);

	return temp->vary_length;
}

void Move::arithmeticException(void)
{
	//(*err) (isc_arith_except, 0);
}

void Move::integerToText(const dsc* from, const dsc* to)
{
#ifndef NATIVE_QUAD
	/* For now, this routine does not handle quadwords unless this is
	supported by the platform as a native datatype. */

	if (from->dsc_dtype == dtype_quad)
		//(*err) (isc_badblk, 0);	/* internal error */
		notImplemented();
#endif

	SSHORT pad = 0, decimal = 0, neg = 0;

/* Save (or compute) scale of source.  Then convert source to ordinary
   longword or int64. */

	SCHAR scale = from->dsc_scale;

	if (scale > 0)
		pad = scale;
	else if (scale < 0)
		decimal = 1;

	SINT64 n;
	dsc intermediate;
	MOVE_CLEAR(&intermediate, sizeof(intermediate));
	intermediate.dsc_dtype = dtype_int64;
	intermediate.dsc_length = sizeof(n);
	intermediate.dsc_scale = scale;
	intermediate.dsc_address = (UCHAR*) &n;

	move (from, &intermediate);

/* Check for negation, then convert the number to a string of digits */

	UINT64 u;
	
	if (n >= 0)
		u = n;
	else 
		{
		neg = 1;
		u = -n;
		}

    UCHAR temp[32];
    UCHAR* p = temp;

	do {
		*p++ = (UCHAR) (u % 10) + '0';
		u /= 10;
	} while (u);

	SSHORT l = p - temp;

	/* if scale < 0, we need at least abs(scale)+1 digits, so add
	any leading zeroes required. */
	
	for (; l + scale <= 0; ++l) 
		*p++ = '0';

	/* postassertion: l+scale > 0 */
	fb_assert(l + scale > 0);

	// CVC: also, we'll check for buffer overflow directly.
	fb_assert(temp + sizeof(temp) >= p);

/* Compute the total length of the field formatted.  Make sure it
   fits.  Keep in mind that routine handles both string and varying
   string fields. */

	const SSHORT length = l + neg + decimal + pad;

	if ((to->dsc_dtype == dtype_text && length > to->dsc_length) ||
		(to->dsc_dtype == dtype_cstring && length >= to->dsc_length) ||
		(to->dsc_dtype == dtype_varying
		 && length > (SSHORT) (to->dsc_length - sizeof(USHORT))))
	    conversionError(from);

	UCHAR* q = (to->dsc_dtype == dtype_varying) ?
		to->dsc_address + sizeof(USHORT) : to->dsc_address;

/* If negative, put in minus sign */

	if (neg)
		*q++ = '-';

/* If a decimal point is required, do the formatting.  Otherwise just
   copy number */

	if (scale >= 0)
		do {
			*q++ = *--p;
		} while (--l);
	else {
		l += scale;				/* l > 0 (see postassertion: l+scale > 0 above) */
		do {
			*q++ = *--p;
		} while (--l);
		*q++ = '.';
		do {
			*q++ = *--p;
		} while (++scale);
	}

/* If padding is required, do it now. */

	if (pad)
		do {
			*q++ = '0';
		} while (--pad);

/* Finish up by padding (if fixed) or computing the actual length
   (varying string) */

	if (to->dsc_dtype == dtype_text) {
		if ((l = to->dsc_length - length) > 0) {
			do {
				*q++ = ' ';
			} while (--l);
		}
		return;
	}

	if (to->dsc_dtype == dtype_cstring) {
		*q = 0;
		return;
	}

	*(SSHORT *) (to->dsc_address) = q - to->dsc_address - sizeof(SSHORT);
}

void Move::datetimeToText(const dsc* from, const dsc* to)
{
	bool version4 = true;

	fb_assert(DTYPE_IS_TEXT(to->dsc_dtype));

/* Convert a date or time value into a timestamp for manipulation */

	GDS_TIMESTAMP date;
	
	switch (from->dsc_dtype) 
		{
		case dtype_sql_time:
			date.timestamp_date = 0;
			date.timestamp_time = *(GDS_TIME *) from->dsc_address;
			break;
			
		case dtype_sql_date:
			date.timestamp_date = *(GDS_DATE *) from->dsc_address;
			date.timestamp_time = 0;
			break;
			
		case dtype_timestamp:
			if (allowDateStringTruncation())
				version4 = true;
			date = *(GDS_TIMESTAMP *) from->dsc_address;
			break;
			
		default:
			notImplemented();
		}

	/* Decode the timestamp into human readable terms */

	tm times;
	//isc_decode_timestamp(&date, &times);
	decodeTimestamp (&date, &times);
	
	TEXT temp[30];			/* yyyy-mm-dd hh:mm:ss.tttt  OR
							dd-MMM-yyyy hh:mm:ss.tttt */
	TEXT* p = temp;

	/* Make a textual date for data types that include it */

	if (from->dsc_dtype != dtype_sql_time) 
		{
		if (from->dsc_dtype == dtype_sql_date || !version4) 
			{
			sprintf(p, "%4.4d-%2.2d-%2.2d",
					times.tm_year + 1900, times.tm_mon + 1, times.tm_mday);
			}
		else 
			{
			/* Prior to BLR version 5 - timestamps where converted to
			   text in the dd-Mon-yyyy format */
			sprintf(p, "%d-%.3s-%d",
					times.tm_mday,
					months[times.tm_mon], times.tm_year + 1900);
			}
		while (*p)
			p++;
		};

/* Put in a space to separate date & time components */

	if ((from->dsc_dtype == dtype_timestamp) && (!version4))
		*p++ = ' ';

/* Add the time part for data types that include it */

	if (from->dsc_dtype != dtype_sql_date) 
		{
		if (from->dsc_dtype == dtype_sql_time || !version4) 
			{
			sprintf(p, "%2.2d:%2.2d:%2.2d.%4.4d",
					times.tm_hour, times.tm_min, times.tm_sec,
					(USHORT) (date.timestamp_time %
							  ISC_TIME_SECONDS_PRECISION));
			}
		else if (times.tm_hour || times.tm_min || times.tm_sec
				 || date.timestamp_time)
			{
			/* Timestamp formating prior to BLR Version 5 is slightly
			   different */
			sprintf(p, " %d:%.2d:%.2d.%.4d",
					times.tm_hour, times.tm_min, times.tm_sec,
					(USHORT) (date.timestamp_time %
							  ISC_TIME_SECONDS_PRECISION));
			}
		while (*p)
			p++;
	};

/* Move the text version of the date/time value into the destination */

	dsc desc;
	MOVE_CLEAR(&desc, sizeof(desc));
	desc.dsc_address = (UCHAR *) temp;
	desc.dsc_dtype = dtype_text;
	desc.dsc_ttype = ttype_ascii;
	desc.dsc_length = (p - temp);
	
	if (from->dsc_dtype == dtype_timestamp && version4) 
		{
		/* Prior to BLR Version5, when a timestamp is converted to a string it
		   is silently truncated if the destination string is not large enough */

		fb_assert(to->dsc_dtype <= dtype_any_text);

		const USHORT l = (to->dsc_dtype == dtype_cstring) ? 1 :
			(to->dsc_dtype == dtype_varying) ? sizeof(USHORT) : 0;
		desc.dsc_length = MIN(desc.dsc_length, (to->dsc_length - l));
		}

	move(&desc, to);
}

void Move::floatToText(const dsc* from, const dsc* to)
{
	double d;
	char temp[] = "-1.234567890123456E-300";

	const int to_len = getDescStringLength(to); // length of destination
	const int width = MIN(to_len, (int) sizeof(temp) - 1); // minimum width to print

	int precision;
	
	if (dtype_double == from->dsc_dtype) 
		{
		precision = 16;			/* minimum significant digits in a double */
		d = *(double*) from->dsc_address;
		}
	else 
		{
		fb_assert(dtype_real == from->dsc_dtype);
		precision = 8;			/* minimum significant digits in a float */
		d = (double) *(float*) from->dsc_address;
		}

	/* If this is a double with non-zero scale, then it is an old-style
	NUMERIC(15, -scale): print it in fixed format with -scale digits
	to the right of the ".". */

	/* CVC: Here sprintf was given an extra space in the two formatting
			masks used below, "%- #*.*f" and "%- #*.*g" but certainly with positive
			quantities and CAST it yields an annoying leading space.
			However, by getting rid of the space you get in dialect 1:
			cast(17/13 as char(5))  => 1.308
			cast(-17/13 as char(5)) => -1.31
			Since this is inconsistent with dialect 3, see workaround at the tail
			of this function. */

	int chars_printed;			/* number of characters printed */
	
	if ((dtype_double == from->dsc_dtype) && (from->dsc_scale < 0))
		chars_printed = sprintf(temp, "%- #*.*f", width, -from->dsc_scale, d);
	else
		chars_printed = LONG_MAX_int;	/* sure to be greater than to_len */

	/* If it's not an old-style numeric, or the f-format was too long for the
	destination, try g-format with the maximum precision which makes sense
	for the input type: if it fits, we're done. */

	if (chars_printed > width) 
		{
		char num_format[] = "%- #*.*g";
		chars_printed = sprintf(temp, num_format, width, precision, d);

		/* If the full-precision result is too wide for the destination,
		   reduce the precision and try again. */

		if (chars_printed > width) 
			{
			precision -= (chars_printed - width);

			/* If we cannot print at least two digits, one on each side of the
			   ".", report an overflow exception. */
			   
			if (precision < 2)
				arithmeticException();

			chars_printed = sprintf(temp, num_format, width, precision, d);

			/* It's possible that reducing the precision caused sprintf to switch
			   from f-format to e-format, and that the output is still too long
			   for the destination.  If so, reduce the precision once more.
			   This is certain to give a short-enough result. */

			if (chars_printed > width) 
				{
				precision -= (chars_printed - width);
				if (precision < 2)
					arithmeticException();
			    chars_printed = sprintf(temp, num_format, width, precision, d);
				}
			}
		}
	fb_assert(chars_printed <= width);

	/* Now move the result to the destination array. */

	dsc intermediate;
	intermediate.dsc_dtype = dtype_text;
	intermediate.dsc_ttype = ttype_ascii;
	
	/* CVC: If you think this is dangerous, replace the "else" with a call to
			MEMMOVE(temp, temp + 1, chars_printed) or something cleverer.
			Paranoid assumption:
			UCHAR is unsigned char as seen on jrd\common.h => same size. */
			
	if (d < 0)
		{
		intermediate.dsc_address = reinterpret_cast<UCHAR*>(temp);
		intermediate.dsc_length = chars_printed;
		}
	else
		{
		if (!temp[0])
			temp[1] = 0;
		intermediate.dsc_address = reinterpret_cast<UCHAR*>(temp) + 1;
		intermediate.dsc_length = chars_printed - 1;
		}

	move(&intermediate, to);
}

void Move::moveToString(const dsc* from, const dsc* to)
{
	CHARSET_ID charset1 = getCharacterSet (from);
	char* p = (char*) to->dsc_address;
	const char* q = (char*) from->dsc_address;
	
	switch (from->dsc_dtype) 
		{
		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
			{
			CHARSET_ID charset2 = getCharacterSet (to);
			if ((charset1 != charset2) &&
				(charset2 != ttype_none) &&
				(charset1 != ttype_binary) &&
				(charset2 != ttype_binary) &&
				(charset1 != ttype_dynamic) && 
				(charset2 != ttype_dynamic))
				{
				moveInternational (from, to);
				return;
				}

			char *ptr;
			USHORT strtype;
			int length = getStringPointer (from, &strtype, &ptr, NULL, 0);
			int l = length;
			q = ptr;
				
			switch (to->dsc_dtype) 
				{
				case dtype_text:
					{
					length = MIN(length, to->dsc_length);
					l -= length;
					int fill = to->dsc_length - length;
					CVT_COPY_BUFF(q, p, length);
					if (fill > 0) 
						memset (p, (charset2 == ttype_binary)? 0 : ASCII_SPACE, fill);
					}
					break;

				case dtype_cstring:
					/* Note: Following is only correct for narrow and
					multibyte character sets which use a zero
					byte to represent end-of-string */

					length = MIN(length, to->dsc_length - 1);
					l -= length;
					CVT_COPY_BUFF(q, p, length);
					*p = 0;
					break;

				case dtype_varying:
					length = MIN(length, (SLONG) (to->dsc_length - sizeof(USHORT)));
					l -= length;
					/* TMN: Here we should really have the following fb_assert */
					/* fb_assert(length <= MAX_USHORT); */
					((vary*) p)->vary_length = (USHORT) length;
					p = ((vary*) p)->vary_string;
					CVT_COPY_BUFF(q, p, length);
					break;
				}

			for (; l; --l)
				if (*q++ != ASCII_SPACE)
					arithmeticException();
		}
		return;

	case dtype_short:
	case dtype_long:
	case dtype_int64:
	case dtype_quad:
		integerToText (from, to);
		return;

	case dtype_real:
	case dtype_double:
	case dtype_d_float:
		floatToText (from, to);
		return;

	case dtype_sql_date:
	case dtype_sql_time:
	case dtype_timestamp:
		datetimeToText (from, to);
		return;

	case dtype_blob:
		conversionError(from);
		return;
		
	default:
		notImplemented();
	}

}

void Move::wishList(void)
{
	/***
	(*err) (isc_wish_list, isc_arg_gds, isc_blobnotsup,
			isc_arg_string, "move", 0);
	***/
}

SLONG Move::getLong(const dsc* desc, int scale)
{
	SLONG value, high, fraction;

	double d;
	SINT64 val64;
	USHORT length;
	TEXT buffer[50];			/* long enough to represent largest long in ASCII */

/* adjust exact numeric values to same scaling */

	if (DTYPE_IS_EXACT(desc->dsc_dtype))
		scale -= desc->dsc_scale;

	const char* p = reinterpret_cast<char*>(desc->dsc_address);

	switch (desc->dsc_dtype) 
		{
		case dtype_short:
			value = *((SSHORT *) p);
			break;

		case dtype_long:
			value = *((SLONG *) p);
			break;

		case dtype_int64:
			val64 = *((SINT64 *) p);

			/* adjust for scale first, *before* range-checking the value. */
			if (scale > 0) {
				fraction = 0;
				do {
					if (scale == 1)
						fraction = (SLONG) (val64 % 10);
					val64 /= 10;
				} while (--scale);
				if (fraction > 4)
					val64++;
				/*
				* The following 2 lines are correct for platforms where
				* ((-85 / 10 == -8) && (-85 % 10 == -5)).  If we port to
				* a platform where ((-85 / 10 == -9) && (-85 % 10 == 5)),
				* we'll have to change this depending on the platform.
				*/
				else if (fraction < -4)
					val64--;
			}
			else if (scale < 0)
				do {
					if ((val64 > INT64_LIMIT) || (val64 < -INT64_LIMIT))
						arithmeticException();
					val64 *= 10;
				} while (++scale);

			if ((val64 > LONG_MAX_int64) || (val64 < LONG_MIN_int64))
				arithmeticException();
			return (SLONG) val64;

		case dtype_quad:
			value = ((SLONG *) p)[LOW_WORD];
			high = ((SLONG *) p)[HIGH_WORD];
			if ((value >= 0 && !high) || (value < 0 && high == -1))
				break;
			arithmeticException();
			break;

		case dtype_real:
		case dtype_double:
	#ifdef VMS
		case dtype_d_float:
	#endif
			if (desc->dsc_dtype == dtype_real)
				d = *((float*) p);
			else if (desc->dsc_dtype == DEFAULT_DOUBLE)
				d = *((double*) p);
	#ifdef VMS
			else
				d = CNVT_TO_DFLT((double*) p);
	#endif
			if (scale > 0)
				do {
					d /= 10.;
				} while (--scale);
			else if (scale < 0)
				do {
					d *= 10.;
				} while (++scale);
			if (d > 0)
				d += 0.5;
			else
				d -= 0.5;

			/* make sure the cast will succeed - different machines 
			do different things if the value is larger than a long
			can hold */
			/* If rounding would yield a legitimate value, permit it */

			if (d < (double) LONG_MIN_real) {
				if (d > (double) LONG_MIN_real - 1.)
					return LONG_MIN;
				arithmeticException();
			}
			if (d > (double) LONG_MAX_real) {
				if (d < (double) LONG_MAX_real + 1.)
					return LONG_MAX_int;
				arithmeticException();
			}
			return (SLONG) d;

		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
			length = makeString(desc, ttype_ascii, &p, (vary*) buffer, sizeof(buffer));
			scale -= decompose(p, length, dtype_long, &value);
			break;

		case dtype_blob:
		case dtype_sql_date:
		case dtype_sql_time:
		case dtype_timestamp:
		case dtype_array:
			conversionError(desc);
			break;

		default:
			badBlock();	/* internal error */
			break;
		}

	/* Last, but not least, adjust for scale */

	if (scale > 0) 
		{
		if (DTYPE_IS_EXACT(desc->dsc_dtype)) 
			{
			fraction = 0;
			do {
				if (scale == 1)
					fraction = value % 10;
				value /= 10;
			} while (--scale);
			if (fraction > 4)
				value++;
			/*
			 * The following 2 lines are correct for platforms where
			 * ((-85 / 10 == -8) && (-85 % 10 == -5)).  If we port to
			 * a platform where ((-85 / 10 == -9) && (-85 % 10 == 5)),
			 * we'll have to change this depending on the platform.
			 */
			else if (fraction < -4)
				value--;
			}
		else 
			{
			do {
				value /= 10;
			} while (--scale);
			}
		}
	else if (scale < 0) 
		{
		do {
			if (value > LONG_LIMIT || value < -LONG_LIMIT)
				arithmeticException();
			value *= 10;
		} while (++scale);
		}

	return value;
}

INT64 Move::getInt64(const dsc* desc, int scale)
{
	SINT64 value;
	SLONG fraction;
	double d;
	USHORT length;
	TEXT buffer[50];			/* long enough to represent largest SINT64 in ASCII */

	/* adjust exact numeric values to same scaling */

	if (DTYPE_IS_EXACT(desc->dsc_dtype))
		scale -= desc->dsc_scale;

	const char* p = reinterpret_cast<char*>(desc->dsc_address);

	switch (desc->dsc_dtype) 
		{
		case dtype_short:
			value = *((SSHORT *) p);
			break;

		case dtype_long:
			value = *((SLONG *) p);
			break;

		case dtype_int64:
			value = *((SINT64 *) p);
			break;

		case dtype_quad:
			value = (((SINT64) ((SLONG *) p)[HIGH_WORD]) << 32) +
				(((ULONG *) p)[LOW_WORD]);
			break;

		case dtype_real:
		case dtype_double:
#ifdef VMS
		case dtype_d_float:
#endif
	
			if (desc->dsc_dtype == dtype_real)
				d = *((float*) p);
			else if (desc->dsc_dtype == DEFAULT_DOUBLE)
				d = *((double*) p);
				
#ifdef VMS
			else
				d = CNVT_TO_DFLT((double*) p);
#endif
	
			if (scale > 0)
				do {
					d /= 10.;
				} while (--scale);
			else if (scale < 0)
				do {
					d *= 10.;
				} while (++scale);
			if (d > 0)
				d += 0.5;
			else
				d -= 0.5;

			/* make sure the cast will succeed - different machines 
			do different things if the value is larger than a quad
			can hold.

			Note that adding or subtracting 0.5, as we do in CVT_get_long,
			will never allow the rounded value to fit into an int64,
			because when the double value is too large in magnitude
			to fit, 0.5 is less than half of the least significant bit
			of the significant (sometimes miscalled "mantissa") of the
			double, and thus will have no effect on the sum. */

			if (d < (double) QUAD_MIN_real || (double) QUAD_MAX_real < d)
				arithmeticException();

			return (SINT64) d;

		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
			length = makeString (desc, ttype_ascii, &p, (vary *) buffer, sizeof(buffer));
			scale -= decompose(p, length, dtype_int64, (SLONG *) & value);
			break;

		case dtype_blob:
		case dtype_sql_date:
		case dtype_sql_time:
		case dtype_timestamp:
		case dtype_array:
			conversionError(desc);
			break;

		default:
			badBlock();
		}

/* Last, but not least, adjust for scale */

	if (scale > 0) 
		{
		if (desc->dsc_dtype == dtype_short ||
			desc->dsc_dtype == dtype_long || desc->dsc_dtype == dtype_int64)
			{
			fraction = 0;
			
			do {
				if (scale == 1)
					fraction = (SLONG) (value % 10);
				value /= 10;
			} while (--scale);
			
			if (fraction > 4)
				value++;
			/*
			 * The following 2 lines are correct for platforms where
			 * ((-85 / 10 == -8) && (-85 % 10 == -5)).  If we port to
			 * a platform where ((-85 / 10 == -9) && (-85 % 10 == 5)),
			 * we'll have to change this depending on the platform.
			 */
			else if (fraction < -4)
				value--;
			}
		else 
			{
			do {
				value /= 10;
			} while (--scale);
			}
		}
	else if (scale < 0) 
		{
		do 
			{
			if (value > INT64_LIMIT || value < -INT64_LIMIT)
				arithmeticException();
			value *= 10;
			} 
		while (++scale);
		}

	return value;
}

SQUAD Move::getQuad(const dsc* desc, int scale)
{
	SQUAD value;
#ifdef NATIVE_QUAD
	SLONG fraction;
#endif
	double d;
	USHORT length;
	TEXT buffer[50];			/* long enough to represent largest quad in ASCII */

/* adjust exact numeric values to same scaling */

	if (DTYPE_IS_EXACT(desc->dsc_dtype))
		scale -= desc->dsc_scale;

	const char* p = reinterpret_cast<char*>(desc->dsc_address);

	switch (desc->dsc_dtype) 
		{
		case dtype_short:
			((SLONG *) & value)[LOW_WORD] = *((SSHORT *) p);
			((SLONG *) & value)[HIGH_WORD] = (*((SSHORT *) p) < 0) ? -1 : 0;
			break;

		case dtype_long:
			((SLONG *) & value)[LOW_WORD] = *((SLONG *) p);
			((SLONG *) & value)[HIGH_WORD] = (*((SLONG *) p) < 0) ? -1 : 0;
			break;

		case dtype_quad:
			value = *((SQUAD *) p);
			break;

		case dtype_int64:
			((SLONG *) & value)[LOW_WORD] = (SLONG) (*((SINT64 *) p) & 0xffffffff);
			((SLONG *) & value)[HIGH_WORD] = (SLONG) (*((SINT64 *) p) >> 32);
			break;

		case dtype_real:
		case dtype_double:
#ifdef VMS
		case dtype_d_float:
#endif
			if (desc->dsc_dtype == dtype_real)
				d = *((float*) p);
			else if (desc->dsc_dtype == DEFAULT_DOUBLE)
				d = *((double*) p);
#ifdef VMS
			else
				d = CNVT_TO_DFLT((double*) p);
#endif
			if (scale > 0)
				do {
					d /= 10.;
				} while (--scale);
			else if (scale < 0)
				do {
					d *= 10.;
				} while (++scale);
			if (d > 0)
				d += 0.5;
			else
				d -= 0.5;

			/* make sure the cast will succeed - different machines 
			do different things if the value is larger than a quad
			can hold */

			if (d < (double) QUAD_MIN_real || (double) QUAD_MAX_real < d) 
				{
				/* If rounding would yield a legitimate value, permit it */

				if (d > (double) QUAD_MIN_real - 1.)
					return QUAD_MIN_int;
				else if (d < (double) QUAD_MAX_real + 1.)
					return QUAD_MAX_int;
				arithmeticException();
				}
			return quadFromDouble (d);

		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
			length = makeString(desc, ttype_ascii, &p, (vary *) buffer, sizeof(buffer));
			scale -= decompose(p, length, dtype_quad, (SLONG*) &value);
			break;

		case dtype_blob:
		case dtype_sql_date:
		case dtype_sql_time:
		case dtype_timestamp:
		case dtype_array:
			conversionError(desc);
			break;

		default:
			badBlock();	/* internal error */
		}

/* Last, but not least, adjust for scale */

	if (scale == 0)
		return value;

#ifndef NATIVE_QUAD
	badBlock();	/* internal error */
#else
	if (scale > 0) {
		if (desc->dsc_dtype == dtype_short ||
			desc->dsc_dtype == dtype_long || desc->dsc_dtype == dtype_quad)
		{
			fraction = 0;
			do {
				if (scale == 1)
					fraction = value % 10;
				value /= 10;
			} while (--scale);
			if (fraction > 4)
				value++;
			/*
			 * The following 2 lines are correct for platforms where
			 * ((-85 / 10 == -8) && (-85 % 10 == -5)).  If we port to
			 * a platform where ((-85 / 10 == -9) && (-85 % 10 == 5)),
			 * we'll have to change this depending on the platform.
			 */
			else if (fraction < -4)
				value--;
		}
		else {
			do {
				value /= 10;
			} while (--scale);
		}
	}
	else {
		do {
			if (value > QUAD_LIMIT || value < -QUAD_LIMIT)
				arithmeticException();
			value *= 10;
		} while (++scale);
	}
#endif

	return value;
}

double Move::getDouble(const dsc* desc)
{
	double value;
	SSHORT scale;

	switch (desc->dsc_dtype) 
		{
		case dtype_short:
			value = *((SSHORT *) desc->dsc_address);
			break;

		case dtype_long:
			value = *((SLONG *) desc->dsc_address);
			break;

		case dtype_quad:
#ifdef NATIVE_QUAD
			value = *((SQUAD *) desc->dsc_address);
#else
			value = ((SLONG *) desc->dsc_address)[HIGH_WORD];
			value *= -((double) LONG_MIN_real);
			if (value < 0)
				value -= ((ULONG *) desc->dsc_address)[LOW_WORD];
			else
				value += ((ULONG *) desc->dsc_address)[LOW_WORD];
#endif
			break;

		case dtype_int64:
			value = (double) *((SINT64 *) desc->dsc_address);
			break;

		case dtype_real:
			return *((float*) desc->dsc_address);

		case DEFAULT_DOUBLE:
			/* a MOVE_FAST is done in case dsc_address is on a platform dependant
			invalid alignment address for doubles */
			MOVE_FAST(desc->dsc_address, &value, sizeof(double));
			return value;

#ifdef VMS
		case SPECIAL_DOUBLE:
			return CNVT_TO_DFLT((double*) desc->dsc_address);
#endif

		case dtype_varying:
		case dtype_cstring:
		case dtype_text:
			{
			TEXT buffer[50];	/* must hold ascii of largest double */
			const char* p;

			const SSHORT length = makeString (desc, ttype_ascii, &p, (vary*) buffer, sizeof(buffer));
			value = 0.0;
			scale = 0;
			SSHORT sign = 0;
			bool digit_seen = false, past_sign = false, fraction = false;
			const char* const end = p + length;
			
			for (; p < end; p++) 
				{
				if (*p == COMMA)
					continue;
				else if (DIGIT(*p)) 
					{
					digit_seen = true;
					past_sign = true;
					if (fraction)
						scale++;
					value = value * 10. + (*p - '0');
					}
				else if (*p == '.') 
					{
					past_sign = true;
					if (fraction)
						conversionError(desc);
					else
						fraction = true;
					}
				else if (!past_sign && *p == '-') 
					{
					sign = -1;
					past_sign = true;
					}
				else if (!past_sign && *p == '+') 
					{
					sign = 1;
					past_sign = true;
					}
				else if (*p == 'e' || *p == 'E')
					break;
				else if (*p != ' ')
					conversionError(desc);
				}

			/* If we didn't see a digit then must be a funny string like "    ".  */
			
			if (!digit_seen)
				conversionError(desc);

			if (sign == -1)
				value = -value;

			/* If there's still something left, there must be an explicit
			  exponent */

			if (p < end) 
				{
				digit_seen = false;
				sign = 0;
				SSHORT exp = 0;
				
				for (p++; p < end; p++) 
					{
					if (DIGIT(*p)) 
						{
						digit_seen = true;
						exp = exp * 10 + *p - '0';

						/* The following is a 'safe' test to prevent overflow of
						exp here and of scale below. A more precise test occurs 
						later in this routine. */

						if (exp >= SHORT_LIMIT)
							arithmeticException();
						}
					else if (*p == '-' && !digit_seen && !sign)
						sign = -1;
					else if (*p == '+' && !digit_seen && !sign)
						sign = 1;
					else if (*p != ' ')
						conversionError(desc);
					}
					
				if (!digit_seen)
					conversionError(desc);

				if (sign == -1)
					scale += exp;
				else
					scale -= exp;
				}

			/* if the scale is greater than the power of 10 representable
			   in a double number, then something has gone wrong... let
			   the user know...  */

			if (ABSOLUT(scale) > DBL_MAX_10_EXP)
				arithmeticException();

			/* Repeated division is a good way to mung the least significant bits
			   of your value, so we have replaced this iterative multiplication/division
			   by a single multiplication or division, depending on sign(scale).
				if (scale > 0)
 					do value /= 10.; while (--scale);
 				else if (scale)
 					do value *= 10.; while (++scale);
			*/
			
			if (scale > 0)
				value /= powerOfTen(scale);
			else if (scale < 0)
				value *= powerOfTen(-scale);
				
			}
			return value;

		case dtype_timestamp:
		case dtype_sql_date:
		case dtype_sql_time:
		case dtype_blob:
		case dtype_array:
			conversionError(desc);
			break;

		default:
			badBlock();
		}

	/* Last, but not least, adjust for scale */

	if ((scale = desc->dsc_scale) == 0)
		return value;

	/* if the scale is greater than the power of 10 representable
	   in a double number, then something has gone wrong... let
	   the user know... */

	if (ABSOLUT(scale) > DBL_MAX_10_EXP)
		arithmeticException();

	if (scale > 0)
		value *= powerOfTen(scale);
	else if (scale < 0)
		value /= powerOfTen(-scale);

	return value;
}

/*
 * Calenders are divided into 4 year cycles.
 * 3 Non-Leap years, and 1 leap year.
 * Each cycle takes 365*4 + 1 == 1461 days.
 * There is a further cycle of 100 4 year cycles.
 * Every 100 years, the normally expected leap year
 * is not present.  Every 400 years it is.
 * This cycle takes 100 * 1461 - 3 == 146097 days
 * The origin of the constant 2400001 is unknown.
 * The origin of the constant 1721119 is unknown.
 * The difference between 2400001 and 1721119 is the
 * number of days From 0/0/0000 to our base date of
 * 11/xx/1858. (678882)
 * The origin of the constant 153 is unknown.
 *
 * This whole routine has problems with ndates
 * less than -678882 (Approx 2/1/0000).
 */
 
void Move::decodeDate(int nday, tm* times)
{
	nday -= 1721119 - 2400001;
	const SLONG century = (4 * nday - 1) / 146097;
	nday = 4 * nday - 1 - 146097 * century;
	SLONG day = nday / 4;

	nday = (4 * day + 3) / 1461;
	day = 4 * day + 3 - 1461 * nday;
	day = (day + 4) / 4;

	SLONG month = (5 * day - 3) / 153;
	day = 5 * day - 3 - 153 * month;
	day = (day + 5) / 5;

	SLONG year = 100 * century + nday;

	if (month < 10)
		month += 3;
	else {
		month -= 9;
		year += 1;
	}

	times->tm_mday = (int) day;
	times->tm_mon = (int) month - 1;
	times->tm_year = (int) year - 1900;
}

/*
 *	Convert a calendar date to the day-of-year.
 *
 *	The unix time structure considers
 *	january 1 to be Year day 0, although it
 *	is day 1 of the month.   (Note that QLI,
 *	when printing Year days takes the other
 *	view.)   
 */

int Move::getDayOfYear(const tm* times)
{
	SSHORT day = times->tm_mday;
	const SSHORT month = times->tm_mon;
	const SSHORT year = times->tm_year + 1900;

	--day;

	day += (214 * month + 3) / 7;

	if (month < 2)
		return day;

	if (year % 4 == 0 && year % 100 != 0 || year % 400 == 0)
		--day;
	else
		day -= 2;

	return day;
}

GDS_DATE Move::getGDSDate(const tm* times)
{
	const SSHORT day = times->tm_mday;
	SSHORT month = times->tm_mon + 1;
	SSHORT year = times->tm_year + 1900;

	if (month > 2)
		month -= 3;
	else {
		month += 9;
		year -= 1;
	}

	const SLONG c = year / 100;
	const SLONG ya = year - 100 * c;

	return (GDS_DATE) (((SINT64) 146097 * c) / 4 +
					   (1461 * ya) / 4 +
					   (153 * month + 2) / 5 + day + 1721119 - 2400001);
}

int Move::makeString(const dsc* desc, int toInterpretation, const char** address, vary* temp, int length)
{
	if (desc->dsc_dtype <= dtype_any_text && INTL_TTYPE(desc) == toInterpretation) 
		{
		*address = reinterpret_cast<char*>(desc->dsc_address);
		int from_len = desc->dsc_length;
		
		if (desc->dsc_dtype == dtype_text)
			return from_len;
			
		if (desc->dsc_dtype == dtype_cstring)
			return MIN((int) strlen((char *) desc->dsc_address), from_len - 1);
			
		if (desc->dsc_dtype == dtype_varying) 
			{
			vary* varying = (vary*) desc->dsc_address;
			*address = varying->vary_string;
			return MIN(varying->vary_length, (int) (from_len - sizeof(USHORT)));
			}
		}

/* Not string data, then  -- convert value to varying string. */

	dsc temp_desc;
	MOVE_CLEAR(&temp_desc, sizeof(temp_desc));
	temp_desc.dsc_length = length;
	temp_desc.dsc_address = (UCHAR *) temp;
	INTL_ASSIGN_TTYPE(&temp_desc, toInterpretation);
	temp_desc.dsc_dtype = dtype_varying;
	move(desc, &temp_desc);
	*address = temp->vary_string;

	return temp->vary_length;
}

bool Move::allowDateStringTruncation(void)
{
	return false;
}

int Move::getDescStringLength(const dsc* desc)
{
	switch (desc->dsc_dtype) 
		{
		case dtype_text:
			return desc->dsc_length;
			
		case dtype_cstring:
			return desc->dsc_length - 1;
			
		case dtype_varying:
			return desc->dsc_length - sizeof(USHORT);
			
		default:
			if (desc->dsc_scale == 0)
				return convertToStringLength(desc->dsc_dtype);
			if (desc->dsc_scale < 0)
				return convertToStringLength(desc->dsc_dtype) + 1;
			return convertToStringLength(desc->dsc_dtype) + desc->dsc_scale;
		}
}

int Move::convertToStringLength(int dsc_type)
{
	if (dsc_type < (sizeof(dtypeLengths) /
					sizeof(dtypeLengths[0])))
		return dtypeLengths [dsc_type];

	fb_assert(FALSE);
	
	return 0;
}

int Move::makeString(dsc* desc, int to_interp, const char** address, vary* temp, int length)
{
	fb_assert(desc != NULL);
	fb_assert(address != NULL);
	fb_assert((((temp != NULL) && (length > 0))
			|| ((INTL_TTYPE(desc) <= dtype_any_text)
				&& (INTL_TTYPE(desc) == to_interp))));

	if (desc->dsc_dtype <= dtype_any_text && INTL_TTYPE(desc) == to_interp) 
		{
		*address = reinterpret_cast<char*>(desc->dsc_address);
		const USHORT from_len = desc->dsc_length;
		
		if (desc->dsc_dtype == dtype_text)
			return from_len;
			
		if (desc->dsc_dtype == dtype_cstring)
			return MIN((USHORT) strlen((char *) desc->dsc_address), from_len - 1);
			
		if (desc->dsc_dtype == dtype_varying) 
			{
			vary* varying = (vary*) desc->dsc_address;
			*address = varying->vary_string;
			return MIN(varying->vary_length, (USHORT) (from_len - sizeof(USHORT)));
			}
		}

/* Not string data, then  -- convert value to varying string. */

	dsc temp_desc;
	MOVE_CLEAR(&temp_desc, sizeof(temp_desc));
	temp_desc.dsc_length = length;
	temp_desc.dsc_address = (UCHAR *) temp;
	INTL_ASSIGN_TTYPE(&temp_desc, to_interp);
	temp_desc.dsc_dtype = dtype_varying;
	move (desc, &temp_desc);
	*address = temp->vary_string;

	return temp->vary_length;
}

int Move::decompose(const char* string, int length, int dtype, SLONG* return_value)
{
#ifndef NATIVE_QUAD

	/* For now, this routine does not handle quadwords unless this is
	   supported by the platform as a native datatype. */

	if (dtype == dtype_quad)
		badBlock();
#endif

	dsc errd;
	MOVE_CLEAR(&errd, sizeof(errd));
	errd.dsc_dtype = dtype_text;
	errd.dsc_ttype = ttype_ascii;
	errd.dsc_length = length;
	errd.dsc_address = reinterpret_cast<UCHAR*>(const_cast<char*>(string));

	SINT64 value = 0;
	SSHORT scale = 0, sign = 0;
	bool digit_seen = false, fraction = false;
	const SINT64 lower_limit = (dtype == dtype_long) ? MIN_SLONG : MIN_SINT64;
	const SINT64 upper_limit = (dtype == dtype_long) ? MAX_SLONG : MAX_SINT64;

	const SINT64 limit_by_10 = upper_limit / 10;	/* used to check for overflow */

	const char* p = string;
	const char* const end = p + length;
	
	for (; p < end; p++)
		{
		if (*p == ',')
			continue;
		else if (DIGIT(*p)) 
			{
			digit_seen = true;

			/* Before computing the next value, make sure there will be
			   no overflow. Trying to detect overflow after the fact is
			   tricky: the value doesn't always become negative after an
			   overflow!  */

			if (value >= limit_by_10) 
				{
				/* possibility of an overflow */
				if (value > limit_by_10)
					arithmeticException();
				else if (((*p > '8') && (sign == -1))
						 || ((*p > '7') && (sign != -1)))
					arithmeticException();
				}

			value = value * 10 + *p - '0';
			if (fraction)
				--scale;
			}
		else if (*p == '.') 
			{
			if (fraction)
				conversionError(&errd);
			else
				fraction = true;
			}
		else if (*p == '-' && !digit_seen && !sign && !fraction)
			sign = -1;
		else if (*p == '+' && !digit_seen && !sign && !fraction)
			sign = 1;
		else if (*p == 'e' || *p == 'E')
			break;
		else if (*p != ' ')
			conversionError(&errd);
		}

	if (!digit_seen)
		conversionError(&errd);

	if ((sign == -1) && value != lower_limit)
		value = -value;

	/* If there's still something left, there must be an explicit exponent */
	
	if (p < end) 
		{
		sign = 0;
		SSHORT exp = 0;
		digit_seen = false;
		
		for (p++; p < end; p++) 
			{
			if (DIGIT(*p)) 
				{
				digit_seen = true;
				exp = exp * 10 + *p - '0';

				/* The following is a 'safe' test to prevent overflow of
				   exp here and of scale below. A more precise test will
				   occur in the calling routine when the scale/exp is
				   applied to the value. */

				if (exp >= SHORT_LIMIT)
					arithmeticException();
				}
			else if (*p == '-' && !digit_seen && !sign)
				sign = -1;
			else if (*p == '+' && !digit_seen && !sign)
				sign = 1;
			else if (*p != ' ')
				conversionError(&errd);
			}
			
		if (sign == -1)
			scale -= exp;
		else
			scale += exp;

		if (!digit_seen)
			conversionError(&errd);

	}

	if (dtype == dtype_long)
		*return_value = (SLONG) value;
	else
		*((SINT64 *) return_value) = value;

	return scale;
}

void Move::badBlock(void)
{
}

double Move::powerOfTen(int scale)
{
	/* Note that we could speed things up slightly by making the auxiliary
	 * arrays global to this source module and replacing this function with
	 * a macro, but the old code did up to 308 multiplies to our 1, and
	 * that seems enough of a speed-up for now.
	 */

	static const double upper_part[] =
		{ 1.e000, 1.e032, 1.e064, 1.e096, 1.e128,
		1.e160, 1.e192, 1.e224, 1.e256, 1.e288
	};

	static const double lower_part[] =
		{ 1.e00, 1.e01, 1.e02, 1.e03, 1.e04, 1.e05,
		1.e06, 1.e07, 1.e08, 1.e09, 1.e10, 1.e11,
		1.e12, 1.e13, 1.e14, 1.e15, 1.e16, 1.e17,
		1.e18, 1.e19, 1.e20, 1.e21, 1.e22, 1.e23,
		1.e24, 1.e25, 1.e26, 1.e27, 1.e28, 1.e29,
		1.e30, 1.e31
	};

	/* The sole caller of this function checks for scale <= 308 before calling,
	 * but we just fb_assert the weakest precondition which lets the code work.
	 * If the size of the exponent field, and thus the scaling, of doubles
	 * gets bigger, increase the size of the upper_part array.
	 */
	fb_assert((scale >= 0) && (scale < 320));

	/* Note that "scale >> 5" is another way of writing "scale / 32",
	 * while "scale & 0x1f" is another way of writing "scale % 32".
	 * We split the scale into the lower 5 bits and everything else,
	 * then use the "everything else" to index into the upper_part array,
	 * whose contents increase in steps of 1e32.
	 */
	 
	return upper_part[scale >> 5] * lower_part[scale & 0x1f];
}

SQUAD Move::quadFromDouble(double a)
{
#ifdef NATIVE_QUAD
	
	return a;
	
#else
	
	badBlock();
	
	return SQUAD();
	
#endif
}
