#ifndef _DVECTOR_H_
#define _DVECTOR_H_

#include "SVector.h"

template <typename E> 
class DVector : public SVector<E>
{
public:
	virtual ~DVector() 
		{ 
		for (int n = 0; n < size(); ++n)
			delete buffer[n];
		}
};

#endif	// _DVECTOR_H_
