#ifndef _STATUSPRINT_H_
#define _STATUSPRINT_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class StatusPrint
{
public:
	StatusPrint(void);
	~StatusPrint(void);

	virtual ISC_STATUS printStatus(const ISC_STATUS* statusVector);
	virtual void putError(const char *text);
	virtual ISC_STATUS interpretStatus(int bufferLength, char* buffer, const ISC_STATUS** vector);
	static void getOSText(int type, int code, int bufferLength, TEXT* buffer);
};

#endif
