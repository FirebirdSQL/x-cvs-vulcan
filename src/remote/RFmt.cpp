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
 *
 */

#include "firebird.h"
#include "RFmt.h"
#include "RFormat.h"


RFmt::RFmt(void)
{
	format = NULL;
}

RFmt::RFmt(RFormat *rFormat)
{
	if (format = rFormat)
		format->addRef();
}

RFmt::~RFmt(void)
{
	if (format)
		format->release();
}

void RFmt::operator =(RFormat* fmt)
{
	setFormat (fmt);
}

RFmt& RFmt::operator =(const RFmt& source)
{
	setFormat (source.format);
	
	return *this;
}

void RFmt::setFormat(RFormat* fmt)
{
	if (fmt == format)
		return;

	if (format)
		format->release();
	
	if (format = fmt)
		format->addRef();
}
