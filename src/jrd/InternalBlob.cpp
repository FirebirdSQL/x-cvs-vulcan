// InternalBlob.cpp: implementation of the InternalBlob class.
//
//////////////////////////////////////////////////////////////////////

#include "fbdev.h"
#include "common.h"
#include "InternalBlob.h"
#include "InternalResultSet.h"
#include "InternalStatement.h"
#include "InternalConnection.h"
#include "SQLError.h"
#include "jrd.h"
#include "ThreadData.h"
#include "blb_proto.h"
#include "blb.h"
#include ".\internalblob.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

InternalBlob::InternalBlob(InternalStatement *stmt, bid* id)
{
	statement = stmt;
	blobId = *id;
	fetched = false;
	string = NULL;
}

InternalBlob::~InternalBlob()
{
	delete string;
}

int InternalBlob::length()
{
	if (!fetched)
		fetchBlob();

	return totalLength;
}

int InternalBlob::getSegment(int offset, int length, void * address)
{
	if (!fetched)
		fetchBlob();

	return Stream::getSegment (offset, length, address);
}

void InternalBlob::fetchBlob()
{
	ISC_STATUS statusVector [20];
	ThreadData threadData (statusVector, statement->connection->attachment);
	blb* blob = BLB_open2(threadData, statement->connection->transaction, 
						  &blobId, 0, NULL);
	fetched = true;
	int len = blob->blb_length;
	
	if (len)
		{
		string = new char[len + 1];
		blob->getData(threadData, len, (UCHAR*) string);
		string[len] = 0;
		putSegment(len, string, false);
		}
		
	BLB_close(threadData, blob);
}

char* InternalBlob::getString()
{
	if (string)
		return string;
		
	if (!fetched)
		fetchBlob();

	if (totalLength)
		return "";
		
	return string;
}

int InternalBlob::getSegmentLength(int pos)
{
	if (!fetched)
		fetchBlob();

	return BinaryBlob::getSegmentLength (pos);
}

void* InternalBlob::getSegment(int pos)
{
	if (!fetched)
		fetchBlob();

	return BinaryBlob::getSegment (pos);
}
