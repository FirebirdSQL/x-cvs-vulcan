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

// Element.h: interface for the Element class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _LINK_H_
#define _LINK_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class Link
{
public:
	virtual void putShort(short value);
	virtual void putInt32(INT32 value);
	virtual void putInt64(INT64 value);
	virtual void putDouble(double value);
	virtual void putCString(int length, const char* string);
	virtual short getShort(void);
	virtual INT32 getInt32(void);
	virtual INT64 getInt64(void);
	virtual double getDouble(void);
	
	virtual void putBytes (int length, const UCHAR *bytes) = 0;
	virtual void flushBlock();						// zero fills current encryption block
	virtual void flushMessage();					// flushes data to network
	
	virtual void getBytes (int length, UCHAR *bytes) = 0;
	virtual void endBlock();						// ignore remainder of encryption block
	virtual int bytesAvailable() = 0;				// bytes available without network access
};

#endif
