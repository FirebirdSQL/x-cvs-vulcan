#include <memory.h>
#include <string.h>
#include <limits.h>
#include "firebird.h"
#include "../jrd/constants.h"
#include "common.h"
#include "JrdMove.h"

#include "../jrd/jrd.h"
#include "../jrd/req.h"
#include "intl_proto.h"

#include "gen/iberror.h"
#include "../jrd/gdsassert.h"
#include "../jrd/thd_proto.h"
#include "err_proto.h"


void JrdMove::notImplemented(void)
{
	fb_assert(FALSE);
}

void JrdMove::conversionError(const dsc* desc)
{
	const char* p;
	TEXT s[40];

	if (desc->dsc_dtype == dtype_blob)
		p = "BLOB";
	else if (desc->dsc_dtype == dtype_array)
		p = "ARRAY";
	else 
		{
        const char* string;
		const USHORT length = makeString (desc, ttype_ascii, &string, (vary*) s, sizeof (s));
			//CVT_make_string(desc, ttype_ascii, &string, (vary *) s, sizeof(s), err);
		p = ERR_string(string, length);
		}

	ERR_post (isc_convert_error, isc_arg_string, p, 0);
}

void JrdMove::dateRangeExceeded(void)
{
	ERR_post (isc_date_range_exceeded, 0);
}

void JrdMove::moveInternational(const dsc* from, const dsc* to)
{
	INTL_convert_string(to, from, ERR_post);
}

CHARSET_ID JrdMove::getCharacterSet(const dsc* desc)
{
	if ((INTL_TTYPE(desc) == ttype_dynamic))
		return INTL_charset(NULL, INTL_TTYPE(desc), ERR_post);
	
	return INTL_TTYPE(desc);
}

void JrdMove::arithmeticException(void)
{
	ERR_post (isc_arith_except, 0);
}

void JrdMove::wishList(void)
{
	ERR_post (isc_wish_list, isc_arg_gds, isc_blobnotsup,
			isc_arg_string, "move", 0);
}

time_t JrdMove::getCurrentTime(void)
{

	/** Cannot call GET_THREAD_DATA because that macro calls 
		BUGCHECK i.e. ERR_bugcheck() which is not part of 
		client library **/
		
	thread_db* tdbb = tdbb = PLATFORM_GET_THREAD_DATA;
	time_t clock;

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

	return clock;
}

bool JrdMove::allowDateStringTruncation(void)
{
	thread_db* tdbb = tdbb = PLATFORM_GET_THREAD_DATA;
	
	if ((tdbb) &&
		(((THDD) tdbb)->thdd_type == THDD_TYPE_TDBB) &&
		tdbb->tdbb_request &&
		(tdbb->tdbb_request->req_flags & req_blr_version4))
		return true;
	
	return false;
}

void JrdMove::badBlock(void)
{
	ERR_post (isc_badblk, 0);
}
