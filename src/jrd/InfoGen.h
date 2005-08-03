// InfoGen.h: interface for the InfoGen class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INFOGEN_H__0BBF8C41_9AA3_4D97_B8BC_C21C67DA5539__INCLUDED_)
#define AFX_INFOGEN_H__0BBF8C41_9AA3_4D97_B8BC_C21C67DA5539__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class InfoGen  
{
public:
	int fini();
	bool putInt (UCHAR item, int value);
	bool putShort (UCHAR item, int value);
	bool putShort (int value);
	bool put (UCHAR item, int length);
	bool put(UCHAR item, int length, const UCHAR* stuff);
	InfoGen(UCHAR *buffer, int bufferLength);
	virtual ~InfoGen();

	UCHAR	*buffer;
	UCHAR	*ptr;
	UCHAR	*yellow;
	bool	full;
	bool putString(UCHAR item, const char* value);
	bool putUnknown(UCHAR item);
	bool putInt(int value);
	bool putByte(UCHAR item, UCHAR stuff);
};

#endif // !defined(AFX_INFOGEN_H__0BBF8C41_9AA3_4D97_B8BC_C21C67DA5539__INCLUDED_)
