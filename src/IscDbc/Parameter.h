// Parameter.h: interface for the Parameter class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey


#if !defined(AFX_PARAMETER_H__BD560E65_B194_11D3_AB9F_0000C01D2301__INCLUDED_)
#define AFX_PARAMETER_H__BD560E65_B194_11D3_AB9F_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class Parameter
{
public:
	Parameter (Parameter *nxt, const char *nam, int namLen, const char *val, int valLen);
	virtual ~Parameter();

	int			nameLength;
	char		*name;
	int			valueLength;
	char		*value;
	Parameter	*next;
};

#endif // !defined(AFX_PARAMETER_H__BD560E65_B194_11D3_AB9F_0000C01D2301__INCLUDED_)
