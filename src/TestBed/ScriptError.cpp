// ScriptError.cpp: implementation of the ScriptError class.
//
//////////////////////////////////////////////////////////////////////

#include <stdarg.h>
#include "stdafx.h"
#include "TestBed.h"
#include "ScriptError.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ScriptError::ScriptError(const char *txt, ...)
{
	va_list		args;
	va_start	(args, txt);
	char		temp [1024];

	vsprintf (temp, txt, args);
	text = temp;
}

ScriptError::~ScriptError()
{

}

const char* ScriptError::getText()
{
	return text;
}
