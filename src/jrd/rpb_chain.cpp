/*
 *	PROGRAM:	Server Code
 *	MODULE:		rpb_chain.cpp
 *	DESCRIPTION:	Keeps track of rpb's, updated_in_place in
 *	        		single transcation
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
 * Created by: Alex Peshkov <peshkoff@mail.ru>
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */
 
#include "fbdev.h"
#include "common.h"
#include "../common/classes/alloc.h"
#include "../jrd/Relation.h"
#include "../jrd/rpb_chain.h"

#ifdef DEBUG_GDS_ALLOC
#define ExecAssert(x) fb_assert(x)
#else  //DEBUG_GDS_ALLOC
#define ExecAssert(x) x
#endif //DEBUG_GDS_ALLOC

int traRpbList::PushRpb(struct record_param* value) 
	{
	if (value->rpb_relation->rel_view_rse || value->rpb_relation->rel_file) 
		return -1;

#ifdef SHARED_CACHE
	Sync sync(&syncObject, "traRpbList::PushRpb");
	sync.lock(Exclusive);
#endif
	
	int pos = add(traRpbListElement(value, ~0));
	int level = -1;
	
	if (pos-- > 0) 
		{
		traRpbListElement& prev = (*this)[pos];
		
		if (prev.lr_rpb->rpb_relation->rel_id == value->rpb_relation->rel_id &&
			 prev.lr_rpb->rpb_number == value->rpb_number) 
			{ 
			// we got the same record once more - mark for refetch
			level = prev.level;
			fb_assert(pos >= level);
			fb_assert((*this)[pos - level].level == 0);
			prev.lr_rpb->rpb_stream_flags |= RPB_s_refetch;
			}
		}
		
	(*this)[++pos].level = ++level;
	
	return level;
}

bool traRpbList::PopRpb(struct record_param* value, int Level) 
	{
	if (Level < 0) 
		return false;

#ifdef SHARED_CACHE
	Sync sync(&syncObject, "traRpbList::PopRpb");
	sync.lock(Exclusive);
#endif
	int pos = -1;
	ExecAssert(find(traRpbListElement(value, Level), pos));
	bool rc = (*this)[pos].lr_rpb->rpb_stream_flags & RPB_s_refetch;
	remove(pos);
	
	return rc;
}
