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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#ifndef PORTINET_H
#define PORTINET_H

#include "Port.h"

#ifndef WIN_NT
#define SOCKET		int
#endif

struct slct
{
	int		slct_width;
	int		slct_count;
	SLONG	slct_time;
	fd_set	slct_fdset;
};

class PortInet :
	public Port
{
public:
	PortInet(int size);
	virtual ~PortInet(void);

	SOCKET			port_handle;		/* handle for connection (from by OS) */
	struct linger	port_linger;		/* linger value as defined by SO_LINGER */
	
	virtual int			accept(struct p_cnct*);
	virtual void		disconnect();
	virtual Port*		receive(Packet*);
	virtual XDR_INT		sendPacket(Packet*);
	virtual XDR_INT		sendPartial(Packet*);
	virtual Port*		connect(Packet*, void(*)(Port*));	// Establish secondary connection 
	virtual Port*		auxRequest(Packet*);			// Request to establish secondary connection
	//void unhookPort(Port* parent);
	static void exitHandler(void* arg);
	PortInet* selectPort(slct* selct);
	PortInet* selectAccept(void);
	int selectWait(slct* selct);
	static PortInet* allocPort(ConfObject* configuration, PortInet* parent);
	int error(const char* function, ISC_STATUS operation, int status);
	void genError(ISC_STATUS status, ...);
	int xdrCreate(xdr_t* xdrs, UCHAR* buffer, int length, xdr_op x_op);
	PortInet* auxConnect(Packet* packet, void (*ast)(Port*));

	int receive(UCHAR* buffer, int buffer_length, short* length);
	bool send(const SCHAR* buffer, int buffer_length);
	int sendFull(Packet* packet);
	bool xdrEndOfRecord(xdr_t* xdrs, bool flushnow);
	int checkHost(char* host_name, char* user_name, struct passwd* passwd);
	bool checkProxy(char* host_name, char* user_name);
	static void signalHandler(void *port);
	int parseHosts(char* file_name, char* host_name, char* user_name);
	int parseLine(char* entry1, char* entry2, char* host_name, char* user_name);
	static void alarmHandler(int x);
	static bool_t getBytes(XDR* xdrs, SCHAR* buff, u_int count);
	static bool_t putBytes(XDR* xdrs, const SCHAR* buff, u_int count);
	static int closeSocket(SOCKET socket);
};
#endif

