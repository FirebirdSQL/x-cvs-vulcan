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
 *
 */

#ifndef RBLOB_H
#define RBLOB_H

#define RBL_eof		1
#define RBL_segment	2
#define RBL_eof_pending	4
#define RBL_create	8


class RDatabase;
class RTransaction;

class RBlob
{
public:
	RBlob(RTransaction *transaction, int size);
	virtual ~RBlob(void);

	//struct blk	rbl_header;
	RDatabase*	rbl_rdb;
	RTransaction*	rbl_rtr;
	RBlob*	rbl_next;
	isc_handle	rbl_handle;
	SLONG		rbl_offset;			/* Apparent (to user) offset in blob */
	USHORT		rbl_id;
	USHORT		rbl_flags;
	UCHAR*		rbl_ptr;
	UCHAR*		rbl_buffer;
	int			rbl_buffer_length;
	int			rbl_initial_length;
	USHORT		rbl_length;
	USHORT		rbl_fragment_length;
	USHORT		rbl_source_interp;	/* source interp (for writing) */
	USHORT		rbl_target_interp;	/* destination interp (for reading) */
	//UCHAR		rbl_data[1];
	UCHAR		*rbl_data;
	
	UCHAR* allocBuffer(int size);
};

#endif

