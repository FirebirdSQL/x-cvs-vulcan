/*
 *	PROGRAM:	Interprocess Interface definitions
 *      MODULE:         xnet.h
 *	DESCRIPTION:	Common descriptions
 *
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
 * 2003.05.01 Victor Seryodkin, Dmitry Yemanov: Completed XNET implementation
 */

#ifndef REMOTE_PORT_XNET_H
#define REMOTE_PORT_XNET_H

#include "Port.h"

class PortXNet :
	public Port
{
public:
	PortXNet(int size);
	virtual ~PortXNet(void);
	static bool_t getBytes(XDR* xdrs, SCHAR* buff, u_int count);
	static bool_t putBytes(XDR* xdrs, const SCHAR* buff, u_int count);
};

#endif

