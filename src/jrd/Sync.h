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

// Sync.h: interface for the Sync class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYNC_H__59333A55_BC53_11D2_AB5E_0000C01D2301__INCLUDED_)
#define AFX_SYNC_H__59333A55_BC53_11D2_AB5E_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


START_NAMESPACE

class LinkedList;
class SynchronizationObject;

class Sync  
{
public:
	void print (const char* label);
	void findLocks (LinkedList &threads, LinkedList& syncObjects);
	void lock (LockType type, const char *fromWhere);
	//void print(int level);
	void setObject (SynchronizationObject *obj);
	void unlock();
	void lock (LockType type);
	void downGrade(LockType type);
	Sync(SynchronizationObject *obj, const char *where);
	virtual ~Sync();

	SynchronizationObject	*syncObject;
	LockType	state;
	LockType	request;
	Sync		*prior;
	const char	*where;
};

END_NAMESPACE

#endif // !defined(AFX_SYNC_H__59333A55_BC53_11D2_AB5E_0000C01D2301__INCLUDED_)
