// RecentStringList.h: interface for the RecentStringList class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RECENTSTRINGLIST_H__68FCDBDC_4C30_43A3_A731_4C6B2DD62729__INCLUDED_)
#define AFX_RECENTSTRINGLIST_H__68FCDBDC_4C30_43A3_A731_4C6B2DD62729__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class RecentStringList  
{
public:
	int getSize();
	void add (CString string);
	void writeList();
	void readList();
	RecentStringList(CString sectionName, CString entryFormat, int max);
	virtual ~RecentStringList();
	CString operator [](int nIndex);

	int		maxStrings;
	CString	*strings;
	CString	section;
	CString	format;
};

#endif // !defined(AFX_RECENTSTRINGLIST_H__68FCDBDC_4C30_43A3_A731_4C6B2DD62729__INCLUDED_)
