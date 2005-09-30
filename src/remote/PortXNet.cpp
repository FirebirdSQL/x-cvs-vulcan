/*
 *	PROGRAM:	JRD Remote Interface/Server
 *      MODULE:         xnet.cpp
 *      DESCRIPTION:    Interprocess Server Communications module.
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

#include <time.h>
#include "fbdev.h"
#include "common.h"
#include "ibase.h"
#include "PortXNet.h"
#include "../jrd/ib_stdio.h"
#include "../remote/remote.h"
#include "../jrd/thd.h"
#include "../jrd/iberr.h"
#include "../remote/xnet.h"
//#include "../utilities/install/install_nt.h"
#include "../remote/proto_proto.h"
#include "../remote/remot_proto.h"
#include "../remote/xnet_proto.h"
#include "../remote/serve_proto.h"
#include "../remote/os/win32/window.h"
//#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
//#include "../jrd/sch_proto.h"
//#include "../jrd/thd_proto.h"
#include "Mutex.h"

#ifdef WIN_NT
#include <windows.h>
#endif /* WIN_NT */

#ifdef WIN_NT
#define ERRNO		GetLastError()
#endif

#ifdef UNIX
#define ERRNO		errno
#endif

#ifndef SYS_ERR
#define SYS_ERR		isc_arg_win32
#endif

#define MAX_SEQUENCE	256

#ifdef SUPERCLIENT
static HANDLE server_process_handle = 0;
static void xnet_on_server_shutdown(PORT port);
#else
static TEXT XNET_command_line[MAXPATHLEN + 32], *XNET_p;
static XPM xnet_make_xpm(ULONG, time_t);
#endif // SUPERCLIENT


static xdr_t::xdr_ops xnet_ops = {
	Port::getLong,			//inet_getlong,
	Port::putLong,			//inet_putlong,
	PortXNet::getBytes,		//xnet_getbytes,
	PortXNet::putBytes,		//xnet_putbytes,
	Port::getPosition,		//inet_getpostn,
	Port::setPosition,		//inet_setpostn,
	Port::inlinePointer,	//inet_inline,
	Port::destroy			// inet_destroy
};

#ifndef MAX_PTYPE
#define MAX_PTYPE	ptype_out_of_band
#endif

static ULONG pages_per_slot = XPS_DEF_PAGES_PER_CLI;
static ULONG slots_per_map = XPS_DEF_NUM_CLI;
static XPM client_maps = NULL;

#ifdef WIN_NT

static HANDLE xnet_connect_mutex = 0;
static HANDLE xnet_connect_map_h = 0;
static CADDR_T xnet_connect_map = 0;

static HANDLE xnet_connect_event = 0;
static HANDLE xnet_response_event = 0;
static DWORD CurrentProcessId;

#endif  /* WIN_NT */

static bool_t xnet_initialized = FALSE;
static bool_t xnet_shutdown = FALSE;
static bool_t xnet_mutex_ready = FALSE;

#ifdef SUPERCLIENT
static bool_t xnet_connect_init();
#endif
static void xnet_connect_fini();
static void xnet_release_all(void);

#ifdef XNET_LOCK
#undef XNET_LOCK
#endif

#ifdef XNET_UNLOCK
#undef XNET_UNLOCK
#endif

//static MUTX_T xnet_mutex;
static Mutex xnet_mutex;

#if defined(SUPERCLIENT)

#define XNET_LOCK		THD_mutex_lock(&xnet_mutex)
#define XNET_UNLOCK		THD_mutex_unlock(&xnet_mutex)

#elif defined(SUPERSERVER)

#define XNET_LOCK		if (!xnet_shutdown)					\
							THD_mutex_lock(&xnet_mutex);

#define XNET_UNLOCK		THD_mutex_unlock(&xnet_mutex)

#else // CS

#define XNET_LOCK
#define XNET_UNLOCK

#endif

PortXNet::PortXNet(int size) : Port (size)
{
}

PortXNet::~PortXNet(void)
{
}

bool_t PortXNet::getBytes(XDR* xdrs, SCHAR* buff, u_int count)
{
	return bool_t();
}

bool_t PortXNet::putBytes(XDR* xdrs, const SCHAR* buff, u_int count)
{
	return bool_t();
}
