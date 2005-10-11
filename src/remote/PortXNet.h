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

#ifndef _WIN32
#ifndef HANDLE
#define HANDLE	int
#endif
#endif

class XNetConnection;
class XNetMappedFile;
//class ConfObject;

class PortXNet : public Port
{
public:
	PortXNet(int size);
	virtual ~PortXNet(void);
	static bool_t getBytes(XDR* xdrs, SCHAR* buff, u_int count);
	static bool_t putBytes(XDR* xdrs, const SCHAR* buff, u_int count);
	static PortXNet* analyze(ConfObject *configuration, TEXT* file_name, USHORT* file_length, ISC_STATUS *status_vector, const TEXT* node_name, const TEXT* user_string, bool uv_flag);
	static PortXNet* connect(ConfObject *configuration, const TEXT* name, Packet* packet, ISC_STATUS *status_vector, int flag);
	PortXNet(PortXNet* parent, XNetConnection *connection);
	virtual void disconnect(void);

	int xdrCreate(XDR* xdrs, UCHAR* buffer, int length, enum xdr_op x_op);
	static void exitHandler(void* arg);
	static void cleanupComm(XNetConnection* xcc);
	static void releaseAll(void);
	static void connectFini(void);
	virtual int accept(p_cnct* cnct);
	virtual Port* receive(Packet* packet);
	virtual XDR_INT sendPacket(Packet* packet);
	virtual XDR_INT sendPartial(Packet* packet);
	virtual Port* connect(Packet* packet, void(* secondaryConnection)(Port*));
	virtual Port* auxRequest(Packet* packet);
	int error(TEXT* function, ISC_STATUS operation, int status, int source_line_num);
	static bool_t write(XDR* xdrs);
	static void logError(int source_line_num, const TEXT* err_msg, int error_code=0);
	void genError(ISC_STATUS status, ...);
	static PortXNet* reconnect(int client_pid, ISC_STATUS *status_vector);
	static PortXNet* getServerPort(PortXNet *parent, int client_pid, XNetMappedFile *xpm, ULONG map_num, ULONG slot_num, time_t timestamp, ISC_STATUS *status_vector);
	static XNetMappedFile* getFreeSlot(ULONG* map_num, ULONG* slot_num, time_t* timestamp);
	static PortXNet* connect(int server_flag, ISC_STATUS *status_vector);
	static void error(const char* operation);
	static bool_t read(XDR* xdrs);

	HANDLE			port_handle;		/* handle for connection (from by OS) */
	XNetConnection	*port_xcc;              /* interprocess structure */
	int				waitCount;
	HANDLE			*waitVector;
	PortXNet		**portVector;
	void init(void);
	virtual void addRef(void);
	virtual void release(void);
};

#endif

