// Stream.h: interface for the Stream class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STREAM_H__02AD6A53_A433_11D2_AB5B_0000C01D2301__INCLUDED_)
#define AFX_STREAM_H__02AD6A53_A433_11D2_AB5B_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

struct Segment
    {
	int		length;
	char	*address;
	Segment	*next;
	};

class Blob;
class Clob;

class Stream  
{
public:
	void putSegment (Blob *blob);
	void putSegment (Clob *blob);
	void* getSegment (int offset);
	int getSegmentLength(int offset);
	void printChars (const char *msg, int length, const char *data);
	void putCharacter (char c);
	void putSegment (int length, const unsigned short *chars);
	void putSegment (Stream *stream);
	void clear();
	void printShorts (const char *msg, int length, short *data);
	char* decompress();
	void compress (int length, void *address);
	virtual char*	getString();
	virtual int		getSegment (int offset, int len, void *ptr, char delimiter);
	virtual void	setSegment (Segment *segment, int length, void *address);
	virtual int		getSegment (int offset, int length, void* address);
	virtual void	putSegment (const char *string);
	virtual void	putSegment (int length, const char *address, bool copy);
	virtual int		getLength();

	char*			alloc (long length);
	Segment*		allocSegment (int tail);
	char*			transferRecord();
	void			setMinSegment (int length);

	Stream();
	Stream (int minSegmentSize);
	virtual ~Stream();

	int		totalLength;
	int		minSegment;
	int		currentLength;
	int		decompressedLength;
	int		useCount;
	bool	copyFlag;
	Segment	first;
	Segment	*segments;
	Segment *current;
};

#endif // !defined(AFX_STREAM_H__02AD6A53_A433_11D2_AB5B_0000C01D2301__INCLUDED_)
