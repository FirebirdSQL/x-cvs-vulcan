// AsciiBlob.cpp: implementation of the AsciiBlob class.
//
//////////////////////////////////////////////////////////////////////

/*
 * copyright (c) 1999 - 2000 by James A. Starkey
 */


#include "firebird.h"
#include "AsciiBlob.h"
#include "BinaryBlob.h"

#ifdef ENGINE
#include "Database.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

AsciiBlob::AsciiBlob()
{
	useCount = 1;
	populated = true;
}


AsciiBlob::AsciiBlob(int minSegmentSize) : Stream (minSegmentSize)
{
	useCount = 1;
	populated = true;
}

AsciiBlob::AsciiBlob(Blob * blob)
{
	useCount = 1;
	populated = true;
	int length = blob->length();
	char *buffer = alloc (length);
	blob->getBytes (0, length, buffer);
}

#ifdef ENGINE
AsciiBlob::AsciiBlob(Database * db, long recNumber, long sectId)
{
	useCount = 1;
	populated = false;
	database = db;
	recordNumber = recNumber;
	sectionId = sectId;
}
#endif

AsciiBlob::~AsciiBlob()
{

}

void AsciiBlob::addRef()
{
	++useCount;
}

int AsciiBlob::release()
{
	if (--useCount == 0)
		{
		delete this;
		return 0;
		}

	return useCount;
}

int AsciiBlob::length()
{
	if (!populated)
		populate();

	return totalLength;
}

void AsciiBlob::getSubString(long pos, long length, char * address)
{
	if (!populated)
		populate();

	Stream::getSegment (pos, length, address);
}

void AsciiBlob::putSegment(int length, const char * data, bool copyFlag)
{
	Stream::putSegment (length, data, copyFlag);
}

int AsciiBlob::getSegmentLength(int pos)
{
	if (!populated)
		populate();

	return Stream::getSegmentLength (pos);
}

const char* AsciiBlob::getSegment(int pos)
{
	if (!populated)
		populate();

	return (const char*) Stream::getSegment (pos);
}

void AsciiBlob::populate()
{
#ifdef ENGINE
	if (database)
		database->fetchRecord (sectionId, recordNumber, this);
#endif

	populated = true;
}

void AsciiBlob::putSegment(const char *string)
{
	Stream::putSegment (string);
}
