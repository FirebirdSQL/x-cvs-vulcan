// Scripts.cpp: implementation of the Scripts class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "Scripts.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Scripts::Scripts(ApplicationObject *parent)
		: ApplicationObject ("Scripts", parent, "Scripts", false)
{
	extensible = true;
	options |= OPTION_ADD_SCRIPT | OPTION_ADD_FOLDER;
}

Scripts::~Scripts()
{

}

Scripts::Scripts()
{

}
