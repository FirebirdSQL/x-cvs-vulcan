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

#include "fbdev.h"
#include "common.h"
#include "ibase.h"
#include "RBlob.h"
#include "remote.h"


RBlob::RBlob(RTransaction *transaction, int size)
{
	rbl_rtr = transaction;
	rbl_rdb = transaction->rtr_rdb;
	rbl_handle = NULL_HANDLE;
	rbl_flags = 0;
	rbl_initial_length = size;
	rbl_data = new UCHAR [rbl_initial_length];
	rbl_buffer_length = rbl_initial_length;
	rbl_buffer = rbl_data;
	rbl_ptr = rbl_buffer;
	rbl_id = 0;
	/***
	SLONG		rbl_offset;	
	USHORT		rbl_id;
	USHORT		rbl_flags;
	UCHAR*		rbl_ptr;
	UCHAR*		rbl_buffer;
	USHORT		rbl_buffer_length;
	USHORT		rbl_length;
	USHORT		rbl_fragment_length;
	USHORT		rbl_source_interp;	// source interp (for writing)
	USHORT		rbl_target_interp;	// destination interp (for reading) 
	***/
}

RBlob::~RBlob(void)
{
	if (rbl_rtr)
		rbl_rtr->releaseBlob (this);
		
	if (rbl_rdb && rbl_id)
		rbl_rdb->rdb_port->releaseObjectId (rbl_id);
		
	delete [] rbl_data;

	if (rbl_buffer != rbl_data)
		delete [] rbl_buffer;
}

UCHAR* RBlob::allocBuffer(int size)
{
	if (size >= rbl_buffer_length)
		{
		if (rbl_buffer != rbl_data)
			delete [] rbl_buffer;
			
		rbl_buffer = new UCHAR [size];
		rbl_buffer_length = size;
		}
	
	return rbl_buffer;
}
