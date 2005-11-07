// Value.cpp: implementation of the Value class.
//
//////////////////////////////////////////////////////////////////////


// copyright (c) 1999 - 2000 by James A. Starkey

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Engine.h"
#include "Value.h"
#include "SQLError.h"
#include "BinaryBlob.h"
#include "AsciiBlob.h"

#define DECIMAL_POINT		'.'
#define DIGIT_SEPARATOR		','

#ifdef _DEBUG
static char THIS_FILE[]=__FILE__;
#endif

#undef NOT_YET_IMPLEMENTED
#define NOT_YET_IMPLEMENTED throw SQLEXCEPTION (FEATURE_NOT_YET_IMPLEMENTED, "conversion is not implemented");

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Value::Value()
{
	type = Null;
}

Value::~Value()
{
	clear();
}


void Value::setValue (long number, int scl)
{
	clear();
	type = Long;
	scale = scl;
	data.integer = number;
}

void Value::setString(const char * string, bool copy)
{
	clear();
	type = String;
	copyFlag = copy;
	data.string.length = strlen (string);

	if (copy)
		{
		data.string.string = new char [data.string.length + 1];
		strcpy (data.string.string, string);
		}
	else
		data.string.string = (char*) string;
}

Value::Value(const char * string)
{
	type = Null;
	setString (string, true);
}

void Value::setValue(Value * value)
{
	switch (value->type)
		{
		case String:
			setString (value->data.string.string, true);
			return;

		case Char:
		case Varchar:
			setString (value->data.string.length, value->data.string.string, true);
			return;
		}

	clear();
	type = value->type;
					
	switch (value->type)
		{
		case Null:
			break;

		case Short:
			scale = value->scale;
			data.integer = value->data.smallInt;
			break;

		case Long:
			scale = value->scale;
			data.integer = value->data.integer;
			break;

		case Quad:
			scale = value->scale;
			data.quad = value->data.quad;
			break;

		case Double:
			data.dbl = value->data.dbl;
			break;

		/***
		case Asciiblob:
			data.blobId = value->data.blobId;
			break;
		
		case Binaryblob:
			data.blobId = value->data.blobId;
			break;
		***/

		case BlobPtr:
			data.blob = value->data.blob;
			data.blob->addRef();
			break;

		case ClobPtr:
			data.clob = value->data.clob;
			data.clob->addRef();
			break;

		case Date:
			data.date = value->data.date;
			break;
									
		default:
			NOT_YET_IMPLEMENTED;
		}			

}

void Value::setString(int length, const char * string, bool copy)
{
	clear();
	type = String;
	copyFlag = copy;
	data.string.length = length;

	if (copy)
		{
		data.string.string = new char [length + 1];
		memcpy (data.string.string, string, length);
		data.string.string [length] = 0;
		}
	else
		data.string.string = (char*) string;
}

char* Value::getString()
{
	switch (type)
		{
		case Null:
			return "";

		case String:
		case Char:
			return data.string.string;

		default:
			NOT_YET_IMPLEMENTED;
		}

	return "";
}


int Value::getString(int bufferSize, char * buffer)
{
	switch (type)
		{
		case String:
		case Char:
		case Varchar:
			if (data.string.length > bufferSize)
				throw SQLEXCEPTION (TRUNCATION_ERROR, "string truncation");
			memcpy (buffer, data.string.string, data.string.length);
			return data.string.length;

		case Date:
			return data.date.getString (bufferSize, buffer);

		case Null:
			buffer [0] = 0;
			return 1;
					
		default:
			NOT_YET_IMPLEMENTED;
		}

	return -1;
}

double Value::getDouble()
{
	switch (type)
		{
		case Null:
			return 0;

		case Double:
			return data.dbl;

		case Short:
		case Long:
		case Quad:
			return (double) getQuad();

		case Char:
		case Varchar:
		case String:
			break;

		case Date:
			return (double) data.date.date;

		default:
			NOT_YET_IMPLEMENTED;
		}

	double divisor;
	QUAD number = convertToQuad (divisor);
	return number / divisor;
}

int Value::compare(Value * value)
{
	if (type == value->type)
		switch (type)
			{
			case Short:
				return data.smallInt - value->data.smallInt;

			case Long:
				return data.integer - value->data.integer;

			case Double:
				return (int) (data.dbl - value->data.dbl);

			case Quad:
				return (int) (data.quad - value->data.quad);
			}

	switch (MAX (type, value->type))
		{
		case Null:
			return 0;

		case String:
		case Char:
		case Varchar:
			{
			char *p = data.string.string;
			char *q = value->data.string.string;
			int l1 = data.string.length;
			int l2 = value->data.string.length;
			int l = MIN (l1, l2);
			int n;
			for (n = 0; n < l; ++n)
				{
				int c = *p++ - *q++;
				if (c)
					return c;
				}
			int c;
			if (n < l1)
				{
				for (; n < l1; ++n)
					if (c = *p++ - ' ')
						return c;
				return 0;
				}
			if (n < l2)
				{
				for (; n < l2; ++n)
					if (c = ' ' - *q++)
						return c;
				}
			return 0;
			}

		case Double:
		case Float:
			return (int) (getDouble() - value->getDouble());

		case Quad:
			return (int) (getQuad() - value->getQuad());

		case Short:
		case Long:
			return (int) (getLong() - value->getLong());

		case Date:
			return getDate().date - value->getDate().date;
		}

	NOT_YET_IMPLEMENTED;

	return 0;						
}

void Value::setValue(double value)
{
	clear();
	type = Double;
	data.dbl = value;
}

QUAD Value::getQuad(int scale)
{
	switch (type)
		{
		case Short:
			return data.smallInt;

		case Long:
			return data.integer;

		case Quad:
			return data.quad;

		case Double:
			return (QUAD) data.dbl;

		default:
			{
			double divisor;
			QUAD quad = convertToQuad (divisor);
			if (scale < 0)
				for (; scale; ++scale)
					divisor /= 10;
			else if (scale > 0)
				for (; scale; --scale)
					divisor *= 10;
			if (divisor == 1)
				return (long) quad;
			return (long) (quad / divisor);
			}
		}

	return 0;
}

long Value::getLong(int scale)
{
	switch (type)
		{
		case Null:
			return 0;

		case Short:
			return data.smallInt;

		case Long:
			return data.integer;

		case Quad:
			return (long) getQuad();

		case Char:
		case Varchar:
		case String:
			{
			double divisor;
			QUAD quad = convertToQuad (divisor);
			if (divisor == 1)
				return (long) quad;
			return (long) (quad / divisor);
			}

		default:
			NOT_YET_IMPLEMENTED;
		}

	return 0;
}

short Value::getShort(int scale)
{
	switch (type)
		{
		case Short:
			return data.smallInt;

		case Long:
			return (short) data.integer;

		case Quad:
		default:
			return (short) getQuad();
		}

	return 0;
}

char* Value::getString(char **tempPtr)
{
	char	temp [64];

	if (tempPtr)
		*tempPtr = NULL;

	switch (type)
		{
		case Null:
			return "";

		case String:
		case Char:
			return data.string.string;

		case Short:
			sprintf (temp, "%d", data.smallInt);
			break;

		case Long:
			sprintf (temp, "%d", data.integer);
			break;

		case Quad:
			convert (data.quad, scale, temp);
			break;

		case Double:
			sprintf (temp, "%f", data.dbl);
			break;

		case Date:
			data.date.getString (sizeof (temp), temp);
			break;

		case Timestamp:
			data.timestamp.getString (sizeof (temp), temp);
			break;

/***
		case AsciiBlob:
		case BinaryBlob:
#ifdef ENGINE
			ASSERT (database);
#endif
***/
		case BlobPtr:
			{
			if (*tempPtr)
				delete [] *tempPtr;
			uint64 length = data.blob->length();
			*tempPtr = new char [(unsigned int) length + 1];
			data.blob->getBytes (0, length, *tempPtr);
			(*tempPtr) [length] = 0;
			return *tempPtr;
			}

		case ClobPtr:
			{
			if (*tempPtr)
				delete [] *tempPtr;
			uint64 length = data.clob->length();
			*tempPtr = new char [(unsigned int) length + 1];
			data.clob->getSubString (0, (unsigned int) length, *tempPtr);
			(*tempPtr) [length] = 0;
			return *tempPtr;
			}

		default:
			NOT_YET_IMPLEMENTED;
		}

	int length = strlen (temp);

	if (*tempPtr)
		delete *tempPtr;

	*tempPtr = new char [length + 1];
	strcpy (*tempPtr, temp);

	return *tempPtr;
}

Blob* Value::getBlob()
{
	BinaryBlob *blob;

	switch (type)
		{
		case Null:
			return new BinaryBlob;

		case BlobPtr:
			data.blob->addRef();
			return data.blob;

		case ClobPtr:
			return new BinaryBlob (data.clob);

		case String:
			blob = new BinaryBlob;
			blob->putSegment (data.string.length, data.string.string, false);	
			return blob;

		/***			
		case AsciiBlob:
		case BinaryBlob:
			blob = new Blob;
			getStream (database, blob, true);	
			return blob;
		***/
		}

	NOT_YET_IMPLEMENTED;

	return NULL;
}


Clob* Value::getClob()
{
	AsciiBlob *blob;

	switch (type)
		{
		case Null:
			return new AsciiBlob;

		case ClobPtr:
			data.clob->addRef();
			return data.clob;

		case BlobPtr:
			return new AsciiBlob (data.blob);

		case String:
			blob = new AsciiBlob;
			blob->putSegment (data.string.length, data.string.string, false);	
			return blob;

		/***			
		case AsciiBlob:
		case BinaryBlob:
			blob = new Blob;
			getStream (database, blob, true);	
			return blob;
		***/
		}

	NOT_YET_IMPLEMENTED;
	return NULL;
}

void Value::getStream(Stream * stream, bool copyFlag)
{
	switch (type)
		{
		case Null:
			break;

		case Char:
		case Varchar:
		case String:
			stream->putSegment (data.string.length, data.string.string, copyFlag);
			break;

/***
		case AsciiBlob:
		case BinaryBlob:
#ifdef ENGINE
			database->fetchRecord (data.blobId.sectionId, data.blobId.recordNumber, stream);
#else
			ASSERT (false);
#endif
			break;
***/

		default:
			{
			char temp [128];
			int length = getString (sizeof (temp), temp);
			stream->putSegment (length, temp, true);
			}
		}
}

void Value::setValue(Blob * blb)
{
	clear();
	type = BlobPtr;
	data.blob = blb;
	data.blob->addRef();
}

/***
void Value::setAsciiBlob(long recordNumber, long sectionId)
{
	clear();
	type = AsciiBlob;
	data.blobId.recordNumber = recordNumber;
	data.blobId.sectionId = sectionId;
}

void Value::setBinaryBlob(long recordNumber, long sectionId)
{
	clear();
	type = BinaryBlob;
	data.blobId.recordNumber = recordNumber;
	data.blobId.sectionId = sectionId;
}
***/

QUAD Value::convertToQuad(double & divisor)
{
	QUAD number = 0;
	divisor = 1;
	bool decimal = false;
	bool negative = false;

	for (char *p = data.string.string, *end = p + data.string.length; p < end;)
		{
		char c = *p++;
		if (c >= '0' && c <= '9')
			{
			number = number * 10 + c - '0';
			if (decimal)
				divisor *= 10;
			}
		else if (c == '-')
			negative = true;
		else if (c == '+' || c == DIGIT_SEPARATOR)
			;
		else if (c == DECIMAL_POINT)
			decimal = true;
		else if (c != ' ' && c != '\t' && c != '\n')
			throw SQLEXCEPTION (CONVERSION_ERROR, "error converting to numeric from '%*s'",
									data.string.length, data.string.string);
		}

	return (negative) ? -number : number;
}

DateTime Value::getDate()
{
	switch (type)
		{
		case Char:
		case String:
		case Varchar:
			break;

		case Date:
			return data.date;

		case Null:
			{
			DateTime date;
			date.setDate(0);
			return date;
			}

		case Long:
			{
			DateTime date;
			date.setDate(data.integer);
			return date;
			}

		case Timestamp:
			return data.timestamp;

		default:
			NOT_YET_IMPLEMENTED;
		}

	return DateTime::convert (data.string.string, data.string.length);
}


TimeStamp Value::getTimestamp()
{
	if (type == Timestamp)
		return data.timestamp;

	TimeStamp timestamp;
	timestamp.setDate(getDate().date);

	return timestamp;
}

void Value::setValue(DateTime value)
{
	clear();
	type = Date;
	data.date = value;
}

void Value::setDate(int value)
{
	clear();
	type = Date;
	data.date.setDate(value);
}

void Value::allocString(Type typ, int length)
{
	clear();
	type = typ;
	data.string.length = length;
	data.string.string = new char [length + 1];
	data.string.string [length] = 0;
}

void Value::setValue(short value, int scl)
{
	clear();
	type = Short;
	scale = scl;
	data.smallInt = value;
}

void Value::setValue(QUAD value, int scl)
{
	clear();
	type = Quad;
	scale = scl;
	data.quad = value;
}

bool Value::isNull()
{
	return type == Null;
}

void Value::setNull()
{
	clear();
}


bool Value::isNull(Type conversionType)
{
	if (type == Null)
		return true;

	if (conversionType == Date)
		switch (type)
			{
			case Char:
			case String:
			case Varchar:
				if (data.string.length == 0)
					return true;
				break;
			}

	return false;
}

void Value::add(Value * value)
{
	Type maxType = MAX (type, value->type);

	if (Null || value->type == Null)
		{
		clear();
		return;
		}

	switch (maxType)
		{
		case Short:
		case Long:
			setValue (getLong() + value->getLong());
			break;

		case Float:
		case Double:
			setValue (getDouble() + value->getDouble());
			break;

		default:
			NOT_YET_IMPLEMENTED;
		}
}

void Value::add(int value)
{
	setValue ((long) (getLong() + value));
}

void Value::divide(Value * value)
{

	if (Null || value->type == Null)
		{
		clear();
		return;
		}

	switch (type)
		{
		case Short:
		case Long:
			{
			long divisor = value->getLong();
			if (divisor == 0)
				throw SQLEXCEPTION (RUNTIME_ERROR, "integer divide by zero");
			setValue (getLong() / divisor);
			}
			break;

		case Float:
		case Double:
			{
			double divisor = value->getLong();
			if (divisor == 0)
				throw SQLEXCEPTION (RUNTIME_ERROR, "integer divide by zero");
			setValue (getDouble() / divisor);
			}
			break;

		default:
			NOT_YET_IMPLEMENTED;
		}
}

char Value::getByte(int scale)
{
	switch (type)
		{
		case Short:
			return (char) data.smallInt;

		case Long:
			return (char) data.integer;

		case Quad:
		default:
			return (char) getQuad();
		}

	return 0;
}

void Value::setValue(TimeStamp value)
{
	clear();
	type = Timestamp;
	data.timestamp = value;
}

Time Value::getTime()
{
	switch (type)
		{
		case Char:
		case String:
		case Varchar:
			break;

		case TimeType:
			return data.time;

		case Null:
			{
			Time date;
			date.setDate(0);
			return date;
			}

		case Long:
			{
			Time date;
			date.date = data.integer;
			return date;
			}

		default:
			NOT_YET_IMPLEMENTED;
		}

	return Time::convert (data.string.string, data.string.length);
}

void Value::setValue(Clob * blob)
{
	clear();
	type = ClobPtr;
	data.clob = blob;
	data.clob->addRef();
}

int Value::convert(QUAD value, int scale, char *string)
{
    if (value == 0)
		{
		strcpy (string, "0");
		return (1);
		}

	if (scale < -18)
		{
		strcpy (string, "***");
		return sizeof ("***");
		}

	bool negative = false;
	UQUAD number;

	if (value > 0)
		number = value;
	else 
		{
		number = -value;
		negative = true;
		}

	char temp [100], *p = temp;
	int n;
	int digits = 0;

	for (n = 0; number; number /= 10, --n)
		{
		if (scale && scale == digits++)
			*p++ = '.';
		*p++ = '0' + (char) (number % 10);
		}

	if (scale && digits <= scale)
		{
		for (; digits < scale; ++digits)
			*p++ = '0';
		*p++ = '.';
		}

	char *q = string;

	if (negative)
		*q++ = '-';

	while (p > temp)
		*q++ = *--p;

	*q = 0;

	return q - string;
}
