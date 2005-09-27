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

#include "fbdev.h"
#include "RMessage.h"
#include "remote.h"


RMessage::RMessage(int size)
{
	msg_buffer = new UCHAR [size];
	msg_address = NULL;
}


RMessage::RMessage(RFormat* format)
{
	msg_format = format;
	msg_buffer = new UCHAR [msg_format->fmt_length];
	msg_address = NULL;
}

RMessage::~RMessage(void)
{
	delete [] msg_buffer;		
}
