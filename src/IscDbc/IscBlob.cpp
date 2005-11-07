// IscBlob.cpp: implementation of the IscBlob class.
//
//////////////////////////////////////////////////////////////////////

#include "IscDbc.h"
#include "IscBlob.h"
#include "IscResultSet.h"
#include "IscStatement.h"
#include "IscConnection.h"
#include "SQLError.h"
#include "SQLError.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscBlob::IscBlob(IscStatement *stmt, ISC_QUAD *id)
{
	statement = stmt;
	blobId = *id;
	fetched = false;
}

IscBlob::~IscBlob()
{

}

uint64 IscBlob::length()
{
	if (!fetched)
		fetchBlob();

	return BinaryBlob::getLength();
}

int IscBlob::getSegment(int offset, int length, void * address)
{
	if (!fetched)
		fetchBlob();

	return Stream::getSegment (offset, length, address);
}

void IscBlob::fetchBlob()
{
	ISC_STATUS statusVector [20];
	IscConnection *connection = statement->connection;
	isc_tr_handle transactionHandle = connection->startTransaction();
	isc_blob_handle blobHandle = NULL;

	int ret = isc_open_blob2 (statusVector, &connection->databaseHandle, &transactionHandle,
							  &blobHandle, &blobId, 0, NULL);

	if (ret)
		THROW_ISC_EXCEPTION (statusVector);

	char buffer [10000];
	unsigned short length;

	for (;;)
		{
		int ret = isc_get_segment (statusVector, &blobHandle, &length, sizeof (buffer), buffer);
		if (ret)
			if (ret == isc_segstr_eof)
				break;
			else if (ret != isc_segment)
				THROW_ISC_EXCEPTION (statusVector);
		putSegment (length, buffer, true);
		}

	isc_close_blob (statusVector, &blobHandle);
	fetched = true;
}

char* IscBlob::getString()
{
	if (!fetched)
		fetchBlob();

	return BinaryBlob::getString ();
}

int IscBlob::getSegmentLength(int pos)
{
	if (!fetched)
		fetchBlob();

	return BinaryBlob::getSegmentLength (pos);
}

void* IscBlob::getSegment(int pos)
{
	if (!fetched)
		fetchBlob();

	return BinaryBlob::getSegment (pos);
}
