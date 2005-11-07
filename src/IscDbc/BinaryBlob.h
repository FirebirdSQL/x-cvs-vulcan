// BinaryBlob.h: interface for the BinaryBlob class.
//
//////////////////////////////////////////////////////////////////////

/*
 * copyright (c) 1999 - 2000 by James A. Starkey
 */


#if !defined(AFX_BINARYBLOB_H__74F68A11_3271_11D4_98E1_0000C01D2301__INCLUDED_)
#define AFX_BINARYBLOB_H__74F68A11_3271_11D4_98E1_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Blob.h"
#include "Stream.h"

class Database;
class AsciiBlob;

class BinaryBlob : public Blob, public Stream 
{
public:
	void putSegment (Blob *blob);
	void putSegment (Clob *blob);
#ifdef ENGINE
	 BinaryBlob (Database *db, long recordNumber, long sectId);
#endif
	virtual void* getSegment (int pos);
	virtual int getSegmentLength (int pos);
	 BinaryBlob (Clob *blob);
	void putSegment (int length, const char *data, bool copyFlag);
	uint64 length();
	void getBytes (uint64 pos, uint64 length, void *address);
	 BinaryBlob (int minSegmentSize);
	BinaryBlob();
	virtual ~BinaryBlob();
	virtual int release();
	virtual void addRef();
	void populate();

	int			useCount;
	int			offset;
	Database	*database;
	long		sectionId;
	long		recordNumber;
	bool		populated;
};

#endif // !defined(AFX_BINARYBLOB_H__74F68A11_3271_11D4_98E1_0000C01D2301__INCLUDED_)
