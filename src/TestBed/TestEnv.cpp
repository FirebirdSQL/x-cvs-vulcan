// TestEnv.cpp: implementation of the TestEnv class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "TestEnv.h"
#include "Scripts.h"
#include "TestBedDoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TestEnv::TestEnv()
{
	scripts = NULL;
}

TestEnv::TestEnv(CTestBedDoc * doc) : ApplicationObject ("TestBed", NULL, "top", false)
{
	scripts = NULL;
	document = doc;
}

TestEnv::~TestEnv()
{
}

void TestEnv::populate()
{
	if (scripts)
		return;

	populated = true;
	scripts = new Scripts (this);	
	addChild (scripts);
}

void TestEnv::markChanged()
{
	document->markChanged();
}

void TestEnv::setDocument(CTestBedDoc * doc)
{
	document = doc;
}

void TestEnv::save(ApplicationObject *object)
{
	if (document)
		document->save();
}
