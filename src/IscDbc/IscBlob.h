// IscBlob.h: interface for the IscBlob class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCBLOB_H__C19738BC_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ISCBLOB_H__C19738BC_1C87_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "BinaryBlob.h"
#include "Connection.h"

class IscStatement;


class IscBlob : public BinaryBlob
{
public:
	virtual void* getSegment (int pos);
	virtual int getSegmentLength (int pos);
	virtual char* getString();
	void fetchBlob();
	virtual int getSegment (int offset, int length, void *address);
	virtual uint64 length();
	IscBlob(IscStatement *parentStatement, ISC_QUAD *id);
	virtual ~IscBlob();

	IscStatement	*statement;
	ISC_QUAD		blobId;
	bool			fetched;
};

#endif // !defined(AFX_ISCBLOB_H__C19738BC_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
