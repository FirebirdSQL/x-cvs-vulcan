// Values.cpp: implementation of the Values class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey


#include "Engine.h"
#include "Values.h"
#include "Value.h"


#ifdef _DEBUG
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Values::Values()
{
	count = 0;
	values = NULL;
}

Values::~Values()
{
	if (values)
		delete [] values;
}

void Values::alloc(int number)
{
	if (number == count)
		{
		for (int n = 0; n < count; ++n)
			values [n].clear();
		return;
		}

	if (values)
		delete [] values;

	count = number;
	values = new Value [count];
}

void Values::clear()
{
	for (int n = 0; n < count; ++n)
		values [n].clear();
}
