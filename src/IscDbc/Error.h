// Error.h: interface for the Error class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey


#if !defined(AFX_ERROR_H__6A019C1E_A340_11D2_AB5A_0000C01D2301__INCLUDED_)
#define AFX_ERROR_H__6A019C1E_A340_11D2_AB5A_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#undef ERROR
#undef ASSERT
#define ERROR	Error::error
#define ASSERT(f)	while (!(f)) Error::assertionFailed (__FILE__, __LINE__)
#define NOT_YET_IMPLEMENTED	ASSERT (false)


class Error  
{
public:
	static void assertionFailed (char *fileName, int line);
	static void error (char *text, ...);
	Error();
	virtual ~Error();

};

#endif // !defined(AFX_ERROR_H__6A019C1E_A340_11D2_AB5A_0000C01D2301__INCLUDED_)
