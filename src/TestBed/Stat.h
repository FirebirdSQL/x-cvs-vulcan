// Stat.h: interface for the Stat class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STAT_H__E11366B9_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_STAT_H__E11366B9_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define INCR(stat,type)		(++stat->counts [type])

#define STAT(type,name)		type,
enum Statistic {
#include "Stats.h"
	stat_max
	};
#undef STAT

class Stat  
{
public:
	Stat();
	virtual ~Stat();

	long	counts [stat_max];
};

#endif // !defined(AFX_STAT_H__E11366B9_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
