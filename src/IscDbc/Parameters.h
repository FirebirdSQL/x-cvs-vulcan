// Parameters.h: interface for the Parameters class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey


#if !defined(AFX_PARAMETERS_H__13461881_1D25_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_PARAMETERS_H__13461881_1D25_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Properties.h"

class Parameter;

class Parameters : public Properties
{
public:
	void clear();
	void copy (Properties *properties);
	virtual const char* getValue (int index);
	virtual const char* getName (int index);
	virtual int getCount();
	virtual const char* findValue (const char *name, const char *defaultValue);
	virtual void putValue(const char * name, int nameLength, const char * value, int valueLength);
	virtual void putValue(const char * name, const char * value);
	Parameters();
	virtual ~Parameters();

	Parameter	*parameters;
	int			count;
};

#endif // !defined(AFX_PARAMETERS_H__13461881_1D25_11D4_98DF_0000C01D2301__INCLUDED_)
