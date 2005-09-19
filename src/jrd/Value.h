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
 *
 *  The Original Code was created by James A. Starkey
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */

// Value.h: interface for the Value class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey


#if !defined(AFX_VALUE_H__02AD6A4B_A433_11D2_AB5B_0000C01D2301__INCLUDED_)
#define AFX_VALUE_H__02AD6A4B_A433_11D2_AB5B_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//#include "Types.h"
#include "TimeStamp.h"
#include "Stream.h"

#include "Blob.h"
#include "DateTime.h"	// Added by ClassView

enum Type {
	Null,
	String,			// generic, null terminated
	Char,			// fixed length string, also null terminated
	Varchar,		// variable length, counted string

	Short,
	Long,
	Quad,

	Float,
	Double,

	Date,
	Timestamp,
	TimeType,

	Asciiblob,		// on disk blob
	Binaryblob,		// on disk blob
	BlobPtr,		// pointer to Blob object
	SqlTimestamp,	// 64 bit version
	ClobPtr
	};

class Blob;
class Clob;
class InternalStatement;
struct dsc;

class Value  
{
public:
	void setValue (Blob *blb);
	Blob* getBlob();
	char* getString (char **tempPtr);
	Value (const char *string);
	int compare (Value *value);

	short	getShort(int scale = 0);
	int		getLong(int scale = 0);
	INT64	getQuad(int scale = 0);
	double	getDouble();
	char	*getString();
	int		getString (int bufferSize, char *buffer);

	void	setValue (double value);
	void	setValue (int value, int scale = 0);
	void	setValue (Value *value);

//protected:
	void	setString (int length, const char *string, bool copy);
	void	setString (const char *value, bool copy);

public:	
	Clob* getClob();
	void setValue (Clob *blob);
	TimeStamp getTimestamp();
	SqlTime getTime();
	void setValue (TimeStamp value);
	char getByte (int scale = 0);
	void divide (Value *value);
	void add (int value);
	void add (Value *value);
	bool isNull (Type type);
	void setDate (int value);
	void setNull();
	bool isNull();
	void setValue (INT64 value, int scale = 0);
	void setValue (short value, int scale = 0);
	void allocString (Type typ, int length);
	void getStream (Stream *stream, bool copyFlag);
	void setValue (DateTime value);
	DateTime getDate();
	INT64 convertToQuad (double& divisor);
	inline void clear()
		{
		if (type == String && copyFlag && data.string.string)
			{
			delete [] data.string.string;
			data.string.string = NULL;
			}
		else if (type == BlobPtr)
			data.blob->release();
		else if (type == ClobPtr)
			data.clob->release();
		type = Null;
		}

	Value();
	virtual ~Value();

	Type		type;
	bool		copyFlag;
	char		scale;

	union
		{
		struct
			{
			char	*string;
			int		length;
			}	string;
		/***
		struct
			{
			long	recordNumber;
			long	sectionId;
			}	blobId;
		***/
		short		smallInt;
		int		integer;
		double		dbl;
		INT64		quad;
		Blob		*blob;
		Clob		*clob;
		DateTime	date;
		TimeStamp	timestamp;
		SqlTime		time;
		} data;
	void setValue(dsc* desc, InternalStatement *statement);
	bool getValue(dsc* desc);
	dsc getDescriptor(void);
};

#endif // !defined(AFX_VALUE_H__02AD6A4B_A433_11D2_AB5B_0000C01D2301__INCLUDED_)
