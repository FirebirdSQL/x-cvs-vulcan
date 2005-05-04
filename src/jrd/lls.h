/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		lls.h
 *	DESCRIPTION:	Linked list stack definitions
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

#ifndef JRD_LLS_H
#define JRD_LLS_H

#include "../jrd/jrd_blks.h"
#include "../include/fb_blk.h"
#include "Stack.h"

class Record;
class jrd_nod;
class RecordSource;
class River;

#define LLS_PUSH(object, stack)		(JrdMemoryPool::ALL_push((BLK)object,stack))
#define LLS_POP(stack)			(JrdMemoryPool::ALL_pop(stack))

class lls : public pool_alloc<type_lls>
{
    public:
        BLK lls_object;
        struct lls *lls_next;
};
typedef lls *LLS;

typedef firebird::Stack<Record*> RecordStack;
typedef firebird::Stack<jrd_nod*> NodeStack;
typedef firebird::Stack<SLONG> PageStack;
typedef firebird::Stack<UCHAR*> UCharStack;
typedef firebird::Stack<RecordSource*> RsbStack;
typedef firebird::Stack<River*> RiverStack;
typedef firebird::Stack<UCHAR> StreamStack;

#endif /* JRD_LLS_H */
