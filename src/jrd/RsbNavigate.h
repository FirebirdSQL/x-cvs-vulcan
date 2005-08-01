/*
 *	PROGRAM:		JRD Access Method
 *	MODULE:			RsbIndexed.h
 *	DESCRIPTION:	Record source block definitions
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
 * Refactored July 25, 2005 by James A. Starkey
 */

#ifndef JRD_RSB_NAVIGATE_H
#define JRD_RSB_NAVIGATE_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RsbIndexed.h"
#include "RecordNumber.h"

struct irsb_nav {
	ULONG irsb_flags;
	SLONG irsb_nav_expanded_offset;			// page offset of current index node on expanded index page
	RecordNumber irsb_nav_number;			// last record number
	SLONG irsb_nav_page;					// index page number
	SLONG irsb_nav_incarnation;				// buffer/page incarnation counter
	ULONG irsb_nav_count;					// record count of last record returned
	RecordBitmap** irsb_nav_bitmap;			// bitmap for inversion tree
	RecordBitmap* irsb_nav_records_visited;	// bitmap of records already retrieved
	USHORT irsb_nav_offset;					// page offset of current index node
	USHORT irsb_nav_lower_length;			// length of lower key value
	USHORT irsb_nav_upper_length;			// length of upper key value
	USHORT irsb_nav_length;					// length of expanded key
	UCHAR irsb_nav_data[1];					// expanded key, upper bound, and index desc
};

typedef irsb_nav *IRSB_NAV;

struct jrd_nod;

class RsbNavigate : public RsbIndexed
{
public:
	RsbNavigate(CompilerScratch *csb, int stream, Relation *relation, str *alias, jrd_nod *node, int key_length);
	virtual ~RsbNavigate(void);
	virtual void	open(Request* request);
	virtual bool	get(Request* request, RSE_GET_MODE mode);
	virtual void	close(Request* request);
	int				computeImpureSize(int key_length);
	
	int		keyLength;
	int		indexOffset;
};

#endif
