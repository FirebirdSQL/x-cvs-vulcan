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

// Synchronize.h: interface for the Synchronize class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYNCHRONIZE_H__9E13C6D8_1F3E_11D3_AB74_0000C01D2301__INCLUDED_)
#define AFX_SYNCHRONIZE_H__9E13C6D8_1F3E_11D3_AB74_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef _PTHREADS
#include <pthread.h>
#endif

#ifdef SOLARIS_MT
#include <thread.h>
#endif

#ifdef ENGINE
#define LOG_DEBUG	Log::debug
#define DEBUG_BREAK	Log::debugBreak
#else
#define LOG_DEBUG	printf
#define DEBUG_BREAK	printf
#endif

#define QUAD	ISC_INT64

class Synchronize  
{
public:
	virtual void shutdown();
	void wake();
	bool sleep (int milliseconds);
	void sleep();
	Synchronize();
	virtual ~Synchronize();

	bool			shutdownInProgress;
	bool			sleeping;
	volatile bool	wakeup;
	QUAD			waitTime;

#ifdef _WIN32
	void	*event;
#endif

#ifdef _PTHREADS
	pthread_cond_t	condition;
	pthread_mutex_t	mutex;
#endif

#ifdef SOLARIS_MT
	cond_t			condition;
	mutex_t			mutex;
#endif
};

#endif // !defined(AFX_SYNCHRONIZE_H__9E13C6D8_1F3E_11D3_AB74_0000C01D2301__INCLUDED_)
