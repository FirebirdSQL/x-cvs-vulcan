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

#ifndef RMESSAGE_H
#define RMESSAGE_H

#include "RefObject.h"
#include "RFmt.h"


class RMessage : public RefObject
{
public:
	RMessage(int size);
	virtual ~RMessage(void);

	RMessage	*msg_next;	/* Next available message */
	
#ifdef SCROLLABLE_CURSORS
	RMessage	*msg_prior;	/* Next available message */
	ULONG		msg_absolute; 		/* Absolute record number in cursor result set */
#endif

	/* Please DO NOT re-arrange the order of following two fields.
	   This could result in alignment problems while trying to access
	   'msg_buffer' as a 'long', leading to "core" drops 
		Sriram - 04-Jun-97 */
		
	int			msg_number;			/* Message number */
	UCHAR		*msg_address;		/* Address of message */
	RFmt		msg_format;
	UCHAR		*msg_buffer;		/* Allocated message */
	RMessage(RFormat* format);
};
#endif

