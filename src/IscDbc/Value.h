// Value.h: interface for the Value class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey


#if !defined(AFX_VALUE_H__02AD6A4B_A433_11D2_AB5B_0000C01D2301__INCLUDED_)
#define AFX_VALUE_H__02AD6A4B_A433_11D2_AB5B_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Types.h"
#include "TimeStamp.h"
#include "Stream.h"

#include "Blob.h"
#include "DateTime.h"	// Added by ClassView

class Blob;
class Clob;

class Value  
{
public:
	//void setBinaryBlob (long recordNumber, long sectionId);
	//void setAsciiBlob (long recordNumber, long sectionId);
	void setValue (Blob *blb);
	Blob* getBlob();
	char* getString (char **tempPtr);
	Value (const char *string);
	int compare (Value *value);

	short	getShort(int scale = 0);
	long	getLong(int scale = 0);
	QUAD	getQuad(int scale = 0);
	double	getDouble();
	char	*getString();
	int		getString (int bufferSize, char *buffer);

	void	setValue (double value);
	void	setValue (long value, int scale = 0);
	void	setValue (Value *value);

//protected:
	void	setString (int length, const char *string, bool copy);
	void	setString (const char *value, bool copy);

public:	
	Clob* getClob();
	void setValue (Clob *blob);
	TimeStamp getTimestamp();
	Time getTime();
	void setValue (TimeStamp value);
	char getByte (int scale = 0);
	void divide (Value *value);
	void add (int value);
	void add (Value *value);
	bool isNull (Type type);
	void setDate (int value);
	void setNull();
	bool isNull();
	void setValue (QUAD value, int scale = 0);
	void setValue (short value, int scale = 0);
	void allocString (Type typ, int length);
	void getStream (Stream *stream, bool copyFlag);
	void setValue (DateTime value);
	DateTime getDate();
	QUAD convertToQuad (double& divisor);
	int convert(QUAD value, int scale, char *string);
	
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
		long		integer;
		double		dbl;
		QUAD		quad;
		Blob		*blob;
		Clob		*clob;
		DateTime	date;
		TimeStamp	timestamp;
		Time		time;
		} data;
};

#endif // !defined(AFX_VALUE_H__02AD6A4B_A433_11D2_AB5B_0000C01D2301__INCLUDED_)
