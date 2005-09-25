/*
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 2005 James A. Starkey
 *  All Rights Reserved.
 */

#include "firebird.h"
#include "../jrd/common.h"
#include "Resource.h"
#include "Relation.h"
#include "Procedure.h"

Resource::Resource(Relation *rel)
{
	relation = rel;
	parentId = relation->rel_id;
	type = rsc_relation;
	procedure = NULL;
}


Resource::Resource(Relation* rel, int id)
{
	relation = rel;
	parentId = id;
	type = rsc_index;
	procedure = NULL;
}

Resource::Resource(Procedure *proc)
{
	procedure = proc;
	parentId = procedure->findId();
	type = rsc_procedure;
	relation = NULL;
}

Resource::~Resource(void)
{
}

Resource* Resource::clone(void)
{
	switch (type)
		{
		case rsc_relation:
			return new Resource(relation);
		
		case rsc_procedure:
			return new Resource(procedure);
		
		case rsc_index:
			return new Resource(relation, parentId);
		
		default:
			fb_assert(false);
		}
	
	return NULL;
}
