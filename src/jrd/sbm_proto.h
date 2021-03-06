/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		sbm_proto.h
 *	DESCRIPTION:	Prototype header file for sbm.cpp
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

#ifndef JRD_SBM_PROTO_H
#define JRD_SBM_PROTO_H

#include "../jrd/sbm.h"
#include "../jrd/rse.h"

#ifdef DEV_BUILD
class SparseBitmap;
#include "../jrd/ib_stdio.h"
void SBM_dump(IB_FILE *, SparseBitmap*);
#endif

class sbm;
class SparseBitmap;
struct thread_db;

SparseBitmap**	SBM_and(SparseBitmap**, SparseBitmap**);
int				SBM_clear(SparseBitmap*, SLONG);
BOOLEAN			SBM_equal(SparseBitmap*, SparseBitmap*);
void			SBM_init(void);
int				SBM_next(SparseBitmap*, SLONG*, RSE_GET_MODE);
SparseBitmap**	SBM_or(thread_db*, SparseBitmap**, SparseBitmap**);
void			SBM_release(SparseBitmap*);
void			SBM_reset(SparseBitmap**);
void			SBM_set(thread_db*, SparseBitmap **, SLONG);
int				SBM_test(SparseBitmap*, SLONG);
SLONG			SBM_size(SparseBitmap**);


#endif // JRD_SBM_PROTO_H

