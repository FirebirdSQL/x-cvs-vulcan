/*
 *  
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
 *  Copyright (c) 1997 - 2000, 2001, 2004 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2004 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

#include "fbdev.h"
#include "common.h"
#include "BdbSort.h"
#include "Bdb.h"

BdbSort::BdbSort(int count)
{
	size = count;
	records = new Bdb* [size];
}

BdbSort::~BdbSort(void)
{
	delete [] records;
}

int BdbSort::compare(Bdb* bdb1, Bdb* bdb2)
{
	return bdb1->bdb_page - bdb2->bdb_page;
}
