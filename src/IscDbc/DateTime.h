// DateTime.h: interface for the DateTime class.
//
//////////////////////////////////////////////////////////////////////

/*
 * copyright (c) 1999 - 2000 by James A. Starkey
 */


#if !defined(AFX_DATETIME_H__84FD1970_A97F_11D2_AB5C_0000C01D2301__INCLUDED_)
#define AFX_DATETIME_H__84FD1970_A97F_11D2_AB5C_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class DateTime  
{
public:
	//DateTime& operator = (QUAD value);
	//DateTime& operator = (long value);
	int getString (const char*format, int length, char *buffer);
	double getDouble();
	static long getNow();
	static long getToday();
	int getString (int length, char *buffer);
	static DateTime conversionError();
	static bool match (const char *str1, const char *str2);
	static int lookup (const char *string, const char **table);
	static DateTime convert (const char *string, int length);
	void setDate(int newDate);

	int date;
};

class Time : public DateTime  
{
public:
	static Time convert (const char *string, int length);
	//Time& operator = (long value);

	int		timeValue;
};

#endif // !defined(AFX_DATETIME_H__84FD1970_A97F_11D2_AB5C_0000C01D2301__INCLUDED_)
