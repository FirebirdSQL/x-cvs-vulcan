/*
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
 * December 27, 2003	Created by James A. Starkey
 *
 */

#ifndef _CharSet_H_
#define _CharSet_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CsConvert.h"

typedef SSHORT CHARSET_ID;
typedef SSHORT COLLATE_ID;

class CharSet
{
public:
	CharSet(struct charset *_cs);
	CharSet(const CharSet &obj);

	int getId() const;
	const char *getName() const;
	UCHAR minBytesPerChar() const;
	UCHAR maxBytesPerChar() const;
	UCHAR getSpaceLength() const;
	const UCHAR *getSpace() const ;

	CsConvert getConvToUnicode();

	CsConvert getConvFromUnicode() ;

	struct charset *getStruct() const;

private:
	struct charset *cs;
public:

	bool isNamed(const char* name);
};

static inline bool operator ==(const CharSet& cs1, const CharSet& cs2) {
	return cs1.getStruct() == cs2.getStruct();
}

static inline bool operator !=(const CharSet& cs1, const CharSet& cs2) {
	return cs1.getStruct() != cs2.getStruct();
}

#endif

