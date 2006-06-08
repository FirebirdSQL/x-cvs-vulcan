// InfoGen.h: interface for the InfoGen class.
//
//////////////////////////////////////////////////////////////////////

#ifndef ENGINEINFOGEN_H
#define ENGINEINFOGEN_H

#include "InfoGen.h"
#include "JString.h"

class EngineInfoGen : public InfoGen 
{
public:
	EngineInfoGen(UCHAR* buff, int bufferLength);
	~EngineInfoGen();

	bool putListBegin();
	bool putListEnd();
	bool putItemValueInt(UCHAR item, int value);
	bool putItemValueString(UCHAR item, const char* value);
};

class EngineInfoReader
{
private:
	UCHAR* buffer;
	UCHAR* ptr;
	UCHAR* end;
public:
	EngineInfoReader(UCHAR* buff, int bufferLength);
	~EngineInfoReader();

	inline int currentPosition();
	inline bool eof();
	UCHAR* getBuffer(int bufferLength);
	inline UCHAR getItem();
	JString getValueString();
	int getValueInt();
	inline int getShort();
	inline bool nextItem();
	void skipItem();
};

#endif // ENGINEINFOGEN_H
