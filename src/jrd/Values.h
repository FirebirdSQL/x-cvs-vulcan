// Values.h: interface for the Values class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey for IBPhoenix.


#ifndef _VALUES_H_
#define _VALUES_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class Value;

class Values  
{
public:
	void clear();
	void alloc (int n);
	Values();
	~Values();

	int		count;
	Value	*values;
};

#endif

