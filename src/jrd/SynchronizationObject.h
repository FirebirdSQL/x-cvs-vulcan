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
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

#ifndef _SYNCHRONIZATIONOBJECT_H_
#define _SYNCHRONIZATIONOBJECT_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


START_NAMESPACE

class Sync;
class LinkedList;

class SynchronizationObject
{
public:
	//SynchronizationObject(void);
	//~SynchronizationObject(void);
	
	virtual void unlock (Sync *sync, LockType type) = 0;
	virtual void lock (Sync *sync, LockType type) = 0;
	virtual void findLocks (LinkedList &threads, LinkedList& syncObjects) = 0;
	virtual void downGrade(LockType type) = 0;
};

END_NAMESPACE

#endif
