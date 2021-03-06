/* $Id$ */
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		all.h
 *	DESCRIPTION:	Block allocator blocks
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

#ifndef JRD_ALL_H
#define JRD_ALL_H

#include "../common/classes/alloc.h"
#include "../jrd/block_cache.h"
#include "../jrd/MemMgr.h"
#include "../include/fb_blk.h"

#ifdef MEMMGR
#include "MemoryManager.h"
#endif

class Database;
class lls;
struct thread_db;

TEXT* ALL_cstring(const TEXT* in_string);
void ALL_fini(void);
void ALL_init(thread_db *tdbb);
//void ALL_push(BLK , LLS *);
//BLK ALL_pop(LLS *);
//void ALL_print_memory_pool_info(IB_FILE*, Database*);

#ifdef DEV_BUILD
void ALL_check_memory(void);
#endif

class Decompress;

class JrdMemoryPool : public MemoryPool
{
protected:
	// Dummy constructor and destructor. Should never be called
#ifdef MEMMGR
#ifdef SHARED_CACHE
	JrdMemoryPool() : MemoryPool(defaultRounding, defaultCutoff, 16384, true), lls_cache(*this) {}
#else
	JrdMemoryPool() : MemoryPool(defaultRounding, defaultCutoff, 16384, false), lls_cache(*this) {}
#endif
#else
	JrdMemoryPool() : MemoryPool(NULL, NULL), lls_cache(*this) {}
#endif
	~JrdMemoryPool() {}	
public:
	static JrdMemoryPool *createPool(Database *dbb, int *cur_mem, int *max_mem);
	static JrdMemoryPool *createPool(Database *dbb);
	static void deletePool(JrdMemoryPool* pool);
	static void noDbbDeletePool(JrdMemoryPool* pool);

	static class blk* ALL_pop(lls**);
	static void       ALL_push(blk*, lls**);

	Decompress* plb_dccs;
private:
	BlockCache<lls> lls_cache;  /* Was plb_lls */
	Database *database;
};

#endif	// JRD_ALL_H
