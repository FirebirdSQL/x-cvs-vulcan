#ifndef _TEMPFILE_H_
#define _TEMPFILE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>
#include "JString.h"

class TempFile
{
public:
	TempFile(void);
	~TempFile(void);
	
	static JString getTempFilename(const char *prototype);
	static FILE* openTempFile(const char* filename, bool autoDelete = false);
	static JString getTempDirectory(void);
};

#endif
