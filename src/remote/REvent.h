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

#ifndef REVENT_H
#define REVENT_H

#include "ibase.h"

class Port;
class RDatabase;

class REvent
{
public:
	REvent(RDatabase *database);
	virtual ~REvent(void);

	//struct blk	rvnt_header;
	REvent			*rvnt_next;
	RDatabase		*rvnt_rdb;
	FPTR_EVENT_CALLBACK	rvnt_ast;
	void*			rvnt_arg;
	ISC_LONG		rvnt_id;
	ISC_LONG		rvnt_rid;	/* used by server to store client-side id */
	Port			*rvnt_port;	/* used to id server from whence async came */
	const UCHAR*	rvnt_items;
	SSHORT			rvnt_length;
};

#endif

