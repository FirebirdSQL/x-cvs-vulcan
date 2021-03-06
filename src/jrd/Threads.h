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

// Threads.h: interface for the Threads class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_THREADS_H__84FD1987_A97F_11D2_AB5C_0000C01D2301__INCLUDED_)
#define AFX_THREADS_H__84FD1987_A97F_11D2_AB5C_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "SyncObject.h"
#include "Synchronize.h"

CLASS (Thread)

START_NAMESPACE

class Threads : public Synchronize
{
public:
	void waitForAll();
	void clear();
	void shutdownAll();
	void exitting (Thread *thread);
	Thread* start (const char *desc, void (fn)(void *), void *arg);
	Threads();
	void release();
	void addRef();

protected:
	virtual ~Threads();

public:
	Thread			*threads;
	SyncObject		syncObject;
	volatile INTERLOCK_TYPE	useCount;
};

END_NAMESPACE

#endif // !defined(AFX_THREADS_H__84FD1987_A97F_11D2_AB5C_0000C01D2301__INCLUDED_)
