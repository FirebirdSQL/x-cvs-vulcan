// RecentStringList.cpp: implementation of the RecentStringList class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MsgMgr.h"
#include "RecentStringList.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

RecentStringList::RecentStringList(CString sectionName, CString entryFormat, int max)
{
	section = sectionName;
	format = entryFormat;
	maxStrings = max;
	strings = new CString [maxStrings];
}

RecentStringList::~RecentStringList()
{
	delete [] strings;
}

void RecentStringList::readList()
{
	CWinApp* pApp = AfxGetApp();

	for (int n = 0; n < maxStrings; ++n)
		{
		CString name;
		name.Format (format, n);
		strings [n] = pApp->GetProfileString (section, name);
		}
}

void RecentStringList::writeList()
{
	CWinApp* pApp = AfxGetApp();
	int sequence = 0;

	for (int n = 0; n < maxStrings; ++n)
		if (!strings [n].IsEmpty())
			{
			CString name;
			name.Format (format, sequence++);
			pApp->WriteProfileString (section, name, strings[n]);
			}
}

void RecentStringList::add (CString string)
{
	int position;

	for (position = 0; position < maxStrings - 1; ++position)
		if (strings [position] == string)
			break;

	for (; position > 0; --position)
		strings [position] = strings [position - 1];

	strings [0] = string;
}

CString RecentStringList::operator [](int nIndex)
{
	return strings [nIndex];
}

int RecentStringList::getSize()
{
	return maxStrings;
}
