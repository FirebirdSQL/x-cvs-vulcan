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

#ifndef RPROCEDURE_H
#define RPROCEDURE_H

#include "RFmt.h"

#define RPR_eof		1		/* End-of-stream encountered */

class RDatabase;
class RTransaction;
class RMessage;
class RFormat;

class RProcedure
{
public:
	RProcedure(RDatabase *database);
	virtual ~RProcedure(void);

	RDatabase*		rpr_rdb;
	RTransaction*	rpr_rtr;
	isc_handle		rpr_handle;
	RMessage*		rpr_in_msg;		/* input message */
	RMessage*		rpr_out_msg;	/* output message */
	RFmt			rpr_in_format;	/* Format of input message */
	RFmt			rpr_out_format;	/* Format of output message */
	USHORT			rpr_flags;
	void clear(void);
};

#endif

