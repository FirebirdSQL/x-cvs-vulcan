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

#ifndef _CsConvert_H_
#define _CsConvert_H_

#include "intlobj.h"



class CsConvert
{ 
public:
	CsConvert() : cnvt(NULL) {}
	CsConvert(struct csconvert *_cnvt) : cnvt(_cnvt) {}
	CsConvert(const CsConvert& obj) : cnvt(obj.cnvt) {}

	USHORT convert(UCHAR *a,
				   USHORT b,
				   UCHAR *c,
				   USHORT d,
				   SSHORT *e,
				   USHORT *f)
	{
		//fb_assert(cnvt != NULL);
		return (*(reinterpret_cast<USHORT (*)(struct csconvert*, UCHAR*,USHORT,
					UCHAR*,USHORT,short*,USHORT*)>(cnvt->csconvert_convert)))
						(cnvt,a,b,c,d,e,f);
	}

	SSHORT getId() const { return cnvt->csconvert_id; }
	const char *getName() const { return cnvt->csconvert_name; }
	CHARSET_ID getFromCS() const { return cnvt->csconvert_from; }
	CHARSET_ID getToCS() const { return cnvt->csconvert_to; }

	struct csconvert *getStruct() const { return cnvt; }

private:
	struct csconvert *cnvt;
};

static inline bool operator ==(const CsConvert& cv1, const CsConvert& cv2) {
	return cv1.getStruct() == cv2.getStruct();
}

static inline bool operator !=(const CsConvert& cv1, const CsConvert& cv2) {
	return cv1.getStruct() != cv2.getStruct();
}

#endif

