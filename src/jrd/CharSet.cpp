/*
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * December 27, 2003	Created by James A. Starkey
 *
 */

#include <string.h>
#include "firebird.h"
#include "common.h"
#include "CharSet.h"

CharSet::CharSet(struct charset *_cs) : cs(_cs) 
	{
	}
	
CharSet::CharSet(const CharSet &obj) : cs(obj.cs) 
	{
	};

int CharSet::getId() const 
	{ 
	return cs->charset_id; 
	}
	
const char *CharSet::getName() const 
	{ 
	return cs->charset_name; 
	}
	
UCHAR CharSet::minBytesPerChar() const 
	{ 
	return cs->charset_min_bytes_per_char; 
	}
	
UCHAR CharSet::maxBytesPerChar() const 
	{ 
	return cs->charset_max_bytes_per_char; 
	}
	
UCHAR CharSet::getSpaceLength() const 
	{ 
	return cs->charset_space_length; 
	}
	
const UCHAR *CharSet::getSpace() const 
	{ 
	return cs->charset_space_character; 
	}

CsConvert CharSet::getConvToUnicode() 
	{ 
	return 
	&cs->charset_to_unicode; 
	}

CsConvert CharSet::getConvFromUnicode() 
	{ 
	return &cs->charset_from_unicode; 
	}

struct charset *CharSet::getStruct() const 
	{ 
	return cs; 
	}

bool CharSet::isNamed(const char* name)
{
	return strcmp (name, cs->charset_name) == 0;
}

