// AsciiBlob.h: interface for the AsciiBlob class.
//
//////////////////////////////////////////////////////////////////////

/*
 * copyright (c) 1999 - 2000 by James A. Starkey
 */

#if !defined(AFX_ASCIIBLOB_H__74F68A12_3271_11D4_98E1_0000C01D2301__INCLUDED_)
#define AFX_ASCIIBLOB_H__74F68A12_3271_11D4_98E1_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Blob.h"
#include "Stream.h"

class Database;


class AsciiBlob : public Clob, public Stream
{
public:
	virtual void putSegment (const char *string);
	AsciiBlob (int minSegmentSize);
	void populate();
	 AsciiBlob(Database * db, long recNumber, long sectId);
	virtual const char* getSegment (int pos);
	virtual int getSegmentLength (int pos);
	AsciiBlob (Blob *blob);
	virtual void putSegment (int length, const char *data, bool copyFlag);
	virtual void getSubString (long pos, long length, char *buffer);
	virtual uint64 length();
	virtual int release();
	virtual void addRef();
	AsciiBlob();
	virtual ~AsciiBlob();

	int		useCount;
	Database	*database;
	long		sectionId;
	long		recordNumber;
	bool		populated;
};

#endif // !defined(AFX_ASCIIBLOB_H__74F68A12_3271_11D4_98E1_0000C01D2301__INCLUDED_)
