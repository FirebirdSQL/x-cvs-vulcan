/*
 *	PROGRAM:		JRD Access Method
 *	MODULE:			RsbExtIndexex.h
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
 * Refactored July 29, 2005 by James A. Starkey
 */

#ifndef JRD_RSB_EXT_DBKEY_H
#define JRD_RSB_EXT_DBKEY_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RsbIndexed.h"

class RsbExtDbkey : public RsbIndexed
{
public:
	RsbExtDbkey(CompilerScratch *csb, int stream, Relation *relation, str *alias, jrd_nod *node);
	virtual ~RsbExtDbkey(void);
	virtual void open(Request* request);
	virtual bool get(Request* request, RSE_GET_MODE mode);
	virtual bool getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen);
	virtual void close(Request* request);
};

#endif

