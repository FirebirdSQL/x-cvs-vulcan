#ifndef _MOVE_H_
#define _MOVE_H_
//#pragma once

#include "dsc.h"
#include "jrd_time.h"
#include "intl.h"
#include "intlobj.h"

enum ExpectDateTime
	{
	expect_timestamp,
	expect_sql_date,
	expect_sql_time
	};

class Move
{
public:
	double		dateToDouble(const dsc *desc);
	void		move(const dsc* from, const dsc* to);
	void		decodeTimestamp(GDS_TIMESTAMP* date, struct tm *times);
	int			getStringPointer(const dsc* desc, USHORT* ttype, char** address, vary* temp, int length);
	void		integerToText(const dsc* from, const dsc* to);
	void		datetimeToText(const dsc* from, const dsc* to);
	void		floatToText(const dsc* from, const dsc* to);
	void		moveToString(const dsc* from, const dsc* to);
	SLONG		getLong(const dsc* from, int scale);
	INT64		getInt64(const dsc* from, int scale);
	SQUAD		getQuad(const dsc* from, int scale);
	double		getDouble(const dsc* from);
	GDS_TIMESTAMP stringToDatetime(const dsc* source, ExpectDateTime expectDatetime);
	virtual int makeString(const dsc* desc, int toInterpretation, const char** address, vary* temp, int length);
	static int	getDescStringLength(const dsc* desc);
	static int	convertToStringLength(int dtype);

	// The following is too simple for engine use
	
	virtual time_t getCurrentTime(void);
	virtual bool allowDateStringTruncation(void);
	
	// The following are not implemented (or no-oped) in the basic implementation
	
	virtual void moveInternational(const dsc* from, const dsc* to);
	virtual CHARSET_ID getCharacterSet(const dsc* desc);

	// The following are useful functions outside the class context
	
	static GDS_TIMESTAMP encodeTimestamp(const struct tm* times);
	static void decodeDate(int nday, tm* times);
	static int getDayOfYear(const tm* times);
	static GDS_DATE getGDSDate(const tm* times);
	
	// The following are exception that may well be overrridden
	
	virtual void arithmeticException(void);
	virtual void conversionError(const dsc* source);
	virtual void wishList(void);
	virtual void dateRangeExceeded(void);
	virtual void notImplemented(void);
	
	int makeString(dsc* desc, int to_interp, const char** address, vary* temp, int length);
	int decompose(const char* string, int length, int dtype, SLONG* return_value);
	virtual void badBlock(void);
	double powerOfTen(int scale);
	SQUAD quadFromDouble(double);
};

#endif
