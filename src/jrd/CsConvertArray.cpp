#include "firebird.h"
#include "common.h"
#include "CsConvertArray.h"

static const int INITIAL_SIZE	= 10;
static const int DELTA_SIZE		= 10;

CsConvertArray::CsConvertArray(void)
{
	count = 0;
	allocated = INITIAL_SIZE;
	vector = new CsConvert [allocated];
}

CsConvertArray::~CsConvertArray(void)
{
	if (vector)
		delete [] vector;
}

bool CsConvertArray::find(int charSetId, int& position)
{
	for (int n = 0; n < count; ++n)
		if (vector [n].getToCS() == charSetId)
			{
			position = n;
			return true;
			}

	return false;
}

CsConvert CsConvertArray::operator [](int index)
{
	return vector [index];
}

void CsConvertArray::add(CsConvert object)
{
	if (count >= allocated)
		extend (count + DELTA_SIZE);
	
	vector [count++] = object;
}

void CsConvertArray::extend(int newSize)
{
	CsConvert *oldVector = vector;
	vector = new CsConvert [newSize];
	
	for (int n = 0; n < count; ++n)
		vector [n] = oldVector [n];
	
	allocated = newSize;
	delete [] oldVector;
}
