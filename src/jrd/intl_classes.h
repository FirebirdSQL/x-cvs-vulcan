/*
 *	PROGRAM:	JRD International support
 *	MODULE:		intl_classes.h
 *	DESCRIPTION:	International text handling definitions
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
 */

#ifndef JRD_INTL_CLASSES_H
#define JRD_INTL_CLASSES_H

#include "firebird.h"
#include "../jrd/jrd.h"
#include "../jrd/constants.h"
#include "../jrd/intlobj.h"

#define TTYPE_TO_CHARSET(tt)    ((SSHORT)((tt) & 0x00FF))
#define TTYPE_TO_COLLATION(tt)  ((SSHORT)((tt) >> 8))

class TextType
{
public:
	TextType(struct texttype *_tt) : tt(_tt) {}

	// copy constructor
	TextType(const TextType& obj) : tt(obj.tt) {}

	USHORT key_length(USHORT a) 
		{
	fb_assert(tt);
		fb_assert(tt->texttype_fn_key_length);
		return (*(reinterpret_cast<USHORT (*)(TEXTTYPE,USHORT)>
					(tt->texttype_fn_key_length)))(tt,a);
	}

	USHORT string_to_key(USHORT a,
						 const UCHAR *b,
						 USHORT c,
						 UCHAR *d,
						 USHORT e)
	{
		fb_assert(tt);
		fb_assert(tt->texttype_fn_string_to_key);
		return (*(reinterpret_cast
			<USHORT(*)(TEXTTYPE,USHORT,const UCHAR*,USHORT,UCHAR*,USHORT)>
				(tt->texttype_fn_string_to_key)))
					(tt,a,b,c,d,e);
	}
	
	SSHORT compare(USHORT a,
				   UCHAR *b,
				   USHORT c,
				   UCHAR *d)
	{
		fb_assert(tt);
		fb_assert(tt->texttype_fn_compare);
		return (*(reinterpret_cast
			<short (*)(TEXTTYPE,USHORT,UCHAR*,USHORT,UCHAR*)>
				(tt->texttype_fn_compare)))(tt,a,b,c,d);
	}
	
	USHORT to_upper(USHORT a)
	{
		fb_assert(tt);
		fb_assert(tt->texttype_fn_to_upper);
		return (*(reinterpret_cast
			<short (*)(TEXTTYPE,USHORT)>
				(tt->texttype_fn_to_upper)))(tt,a);
	}
	
	USHORT to_lower(USHORT a)
	{
		fb_assert(tt);
		fb_assert(tt->texttype_fn_to_lower);
		return (*(reinterpret_cast
			<USHORT (*)(TEXTTYPE,USHORT)>
				(tt->texttype_fn_to_lower)))(tt,a);
	}
	
	SSHORT str_to_upper(USHORT a,
						UCHAR *b,
						USHORT c,
						UCHAR *d)
	{
		fb_assert(tt);
		fb_assert(tt->texttype_fn_str_to_upper);
		return (*(reinterpret_cast
					<short (*)(TEXTTYPE,USHORT,UCHAR*,USHORT,UCHAR*)>
						(tt->texttype_fn_str_to_upper)))
							(tt,a,b,c,d);
	}
	
	USHORT to_wc(UCS2_CHAR *a,
				 USHORT b,
				 const UCHAR *c,
				 USHORT d,
				 SSHORT *e,
				 USHORT *f)
	{
		fb_assert(tt);
		fb_assert(tt->texttype_fn_to_wc);
		return (*(reinterpret_cast
					<USHORT (*)(TEXTTYPE,UCS2_CHAR*,USHORT,const UCHAR*,USHORT,short*,USHORT*)>
						(tt->texttype_fn_to_wc)))
							(tt,a,b,c,d,e,f);
	}
									
	USHORT mbtowc(UCS2_CHAR* a, const UCHAR* b, USHORT c)
	{
		fb_assert(tt);
		fb_assert(tt->texttype_fn_mbtowc);
		return (*(reinterpret_cast<
					USHORT (*)(TEXTTYPE, UCS2_CHAR*, const UCHAR*, USHORT)>
						(tt->texttype_fn_mbtowc)))(tt,a,b,c);
	}

	USHORT contains(struct tdbb* a, const UCHAR *b,
					USHORT c,
					const UCHAR* d,
					USHORT e)
	{
		fb_assert(tt);
		fb_assert(tt->texttype_fn_contains);
		return (*(reinterpret_cast<
					USHORT (*)(struct tdbb*, TextType, const UCHAR*, USHORT, const UCHAR*, USHORT)>
						(tt->texttype_fn_contains)))
							(a,tt,b,c,d,e);
	}
	
	USHORT like(struct tdbb* tdbb, const UCHAR* a,
							  SSHORT b,
							  const UCHAR* c,
							  SSHORT d,
							  SSHORT e)
	{
		fb_assert(tt);
		fb_assert(tt->texttype_fn_like);
		return (*(reinterpret_cast<
					USHORT(*)(struct tdbb*, TextType, const UCHAR*, short, const UCHAR*, short, short)>
						(tt->texttype_fn_like)))(tdbb,tt,a,b,c,d,e);
	}
	
	USHORT matches(struct tdbb* tdbb, const UCHAR* a, SSHORT b, const UCHAR* c, SSHORT d)
	{
		fb_assert(tt);
		fb_assert(tt->texttype_fn_matches);
		return (*(reinterpret_cast<
					USHORT (*)(struct tdbb*, TextType, const UCHAR*, short, const UCHAR*, short)>
						(tt->texttype_fn_matches)))
							(tdbb,tt,a,b,c,d);
	}

	USHORT sleuth_check(struct tdbb* tdbb, USHORT a,
								const UCHAR* b,
								USHORT c,
								const UCHAR* d,
								USHORT e)
	{
		fb_assert(tt);
		fb_assert(tt->texttype_fn_sleuth_check);
		return (*(reinterpret_cast<
					USHORT(*)(struct tdbb*, TextType, USHORT, const UCHAR*, USHORT, const UCHAR*, USHORT)>
						(tt->texttype_fn_sleuth_check)))
							(tdbb,tt,a,b,c,d,e);
	}
	
	USHORT sleuth_merge(struct tdbb* tdbb, const UCHAR* a,
								USHORT b,
								const UCHAR* c,
								USHORT d,
								UCHAR* e,
								USHORT f)
	{
		fb_assert(tt);
		fb_assert(tt->texttype_fn_sleuth_merge);
		return (*(reinterpret_cast<
					USHORT(*)(struct tdbb*, TextType, const UCHAR*, USHORT, const UCHAR*, USHORT, UCHAR*, USHORT)>
						(tt->texttype_fn_sleuth_merge)))
							(tdbb,tt,a,b,c,d,e,f);
	}


	USHORT getType() const {
		fb_assert(tt);
		return tt->texttype_type; 
	}

	const char *getName() const { 
		fb_assert(tt);
		return tt->texttype_name; 
	}

	CHARSET_ID getCharSet() const { 
		fb_assert(tt);
		return tt->texttype_character_set; 
	}

	SSHORT getCountry() const { 
		fb_assert(tt);
		return tt->texttype_country; 
	}

	UCHAR getBytesPerChar() const { 
		fb_assert(tt);
		return tt->texttype_bytes_per_char; 
	}

	struct texttype *getStruct() const { return tt; }

private:
	struct texttype *tt;
};

static inline bool operator ==(const TextType& tt1, const TextType& tt2) {
	return tt1.getStruct() == tt2.getStruct();
}

static inline bool operator !=(const TextType& tt1, const TextType& tt2) {
	return tt1.getStruct() != tt2.getStruct();
}


#include "CsConvert.h"
#include "CharSet.h"

#endif /* JRD_INTL_CLASSES_H */

