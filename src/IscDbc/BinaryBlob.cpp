// BinaryBlob.cpp: implementation of the BinaryBlob class.
//
//////////////////////////////////////////////////////////////////////

/*
 * copyright (c) 1999 - 2000 by James A. Starkey
 */


#include "Engine.h"
#include "BinaryBlob.h"
#include "AsciiBlob.h"

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


BinaryBlob::BinaryBlob()
{
	useCount = 1;
	offset = 0;
	populated = true;
}

BinaryBlob::BinaryBlob(int minSegmentSize) : Stream (minSegmentSize)
{
	useCount = 1;
	offset = 0;
	populated = true;
}

#ifdef ENGINE
BinaryBlob::BinaryBlob(Database * db, long recNumber, long sectId)
{
	useCount = 1;
	offset = 0;
	populated = false;
	database = db;
	recordNumber = recNumber;
	sectionId = sectId;
}
#endif

BinaryBlob::BinaryBlob(Clob * blob)
{
	useCount = 1;
	Stream::putSegment (blob);
}

BinaryBlob::~BinaryBlob()
{

}

void BinaryBlob::addRef()
{
	++useCount;
}

int BinaryBlob::release()
{
	if (--useCount == 0)
		{
		delete this;
		return 0;
		}

	return useCount;
}

/***
bool BinaryBlob::write(const char * filename)
{
	FILE *file = fopen (filename, "w");

	if (!file)
		return false;

	long offset = 0;
	int length;
	char buffer [1024];

	while (length = Stream::getSegment (offset, sizeof (buffer) - 1, buffer))
		{
		buffer [length] = 0;
		fprintf (file, "%s", buffer);
		offset += length;
		}

	fclose (file);

	return true;
}

int BinaryBlob::getLine(int length, char * buffer)
{
	int l = Stream::getSegment (offset, length - 1, buffer, '\n');
	buffer [l] = 0;
	offset += l;

	return l;
}

void BinaryBlob::rewind()
{
	offset = 0;
}

bool BinaryBlob::loadFile(const char * fileName)
{
	FILE *file = fopen (fileName, "r");

	if (!file)
		return false;

	char buffer [256];

	while (fgets (buffer, sizeof (buffer), file))
		putSegment (strlen (buffer), buffer, true);

	fclose (file);

	return true;
}
***/

void BinaryBlob::getBytes(uint64 pos, uint64 length, void * address)
{
	if (!populated)
		populate();

	Stream::getSegment ((int) pos, (int) length, address);
}

uint64 BinaryBlob::length()
{
	if (!populated)
		populate();

	return totalLength;
}

void BinaryBlob::putSegment(int length, const char * data, bool copyFlag)
{
	Stream::putSegment (length, data, copyFlag);
}


int BinaryBlob::getSegmentLength(int pos)
{
	if (!populated)
		populate();

	return Stream::getSegmentLength (pos);
}

void* BinaryBlob::getSegment(int pos)
{
	return Stream::getSegment (pos);
}


void BinaryBlob::populate()
{
#ifdef ENGINE
	if (database)
		database->fetchRecord (sectionId, recordNumber, this);
#endif

	populated = true;
}

void BinaryBlob::putSegment(Blob * blob)
{
	Stream::putSegment (blob);
}

void BinaryBlob::putSegment(Clob * blob)
{
	Stream::putSegment (blob);
}
