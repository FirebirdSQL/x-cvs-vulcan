#ifndef _JRDMOVE_H_
#define _JRDMOVE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Move.h"

class JrdMove :
	public Move
{
public:
	virtual void moveInternational(const dsc* from, const dsc* to);
	virtual CHARSET_ID getCharacterSet(const dsc* desc);

	virtual void arithmeticException(void);
	virtual void conversionError(const dsc* source);
	virtual void wishList(void);
	virtual void dateRangeExceeded(void);
	virtual void notImplemented(void);
	virtual time_t getCurrentTime(void);
	virtual bool allowDateStringTruncation(void);
	virtual void badBlock(void);
};

#endif

