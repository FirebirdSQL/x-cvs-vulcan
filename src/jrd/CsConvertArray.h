#ifndef _CSCONVERTARRAY_H_
#define _CSCONVERTARRAY_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include "intl_classes.h"
#include "JVector.h"
#include "SVector.h"
#include "CsConvert.h"
#include "CharSet.h"

#ifndef TTYPE_TO_CHARSET
#define TTYPE_TO_CHARSET(tt)    ((SSHORT)((tt) & 0x00FF))
#define TTYPE_TO_COLLATION(tt)  ((SSHORT)((tt) >> 8))
#endif

class CharSet;

class CsConvertArray
{
public:
	CsConvertArray(void);
	~CsConvertArray(void);
	
	int			count;
	int			allocated;
	CsConvert	*vector;
	bool find(int charSetId, int& position);
	CsConvert operator [](int index);
	void add(CsConvert object);
	void extend(int newSize);
};


class MemoryPool;
class TextType;
struct tdbb;
class MemoryPool;
class CharSet;
struct tdbb;
//struct texttype;

// Classes and structures used internally to this file and intl implementation

class CharSetContainer
{
public:
	CharSetContainer(tdbb *tdbb, MemoryPool *p, USHORT cs_id);
	CharSet getCharSet() 
		{ 
		return cs; 
		}
	TextType lookupCollation(tdbb* tdbb, USHORT tt_id);
	CsConvert lookupConverter(tdbb* tdbb, int to_cs);
	//static CharSetContainer* lookupCharset(TDBB tdbb, SSHORT ttype, ISC_STATUS *status);
	CharSetContainer	*next;
	int					id;
	
private:
	//NAMESPACE::SortedArray<CsConvert, CHARSET_ID, CharsetIDGetter> charset_converters;
	//NAMESPACE::Array<TextType> charset_collations;
	//NAMESPACE::SortedArray<CHARSET_ID> impossible_conversions;
	JVector				impossible_conversions;
	SVector<texttype*>	charset_collations;
	//JVector			charset_collations;
	CsConvertArray		charset_converters;
	CharSet				cs;
	
public:
	virtual bool isNamed(const char* name);
	virtual bool loadCharSet(tdbb* tdbb, MemoryPool* p, int cs_id);
	virtual int getBytesPerChar(void);
	virtual int getTType(void);
	virtual int getCharsetId(void);
	virtual int getCollationId(void);
	CharSetContainer(void);
};

#endif
