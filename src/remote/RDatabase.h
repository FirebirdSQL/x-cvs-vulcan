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

#ifndef RDATABASE_H
#define RDATABASE_H

#include "protocol.h"
#include "SyncObject.h"
#include "RefObject.h"

class Port;
class RTransaction;
class RRequest;
class RStatement;
class RTransaction;
class REvent;

class RDatabase : public RefObject
{
protected:
	virtual ~RDatabase(void);

public:
	RDatabase(Port *port);

	//struct blk		rdb_header;
	USHORT			rdb_id;
	USHORT			rdb_flags;
	isc_db_handle 	rdb_handle;			/* database handle */
	Port*			rdb_port;			/* communication port */
	RTransaction*	rdb_transactions;	/* linked list of transactions */
	RRequest*		rdb_requests;		/* compiled requests */
	REvent*			rdb_events;			/* known events */
	RStatement*		rdb_sql_requests;	/* SQL requests */
	ISC_STATUS*		rdb_status_vector;
	Packet			rdb_packet;			/* Communication structure */
	SyncObject		syncObject;

	RTransaction* createTransactionId(OBJCT objectId);
	RTransaction* createTransactionHandle(isc_tr_handle transactionHandle);
	void releaseTransaction(RTransaction* transaction);
	void releaseRequest(RRequest* request);
	void releaseStatement(RStatement* statement);
	void clearObjects(void);
	void releaseEvent(REvent* event);
	RRequest* createRequest(int numberMessages);
	REvent* createEvent(void);
	RStatement* createStatement(void);
	REvent* findEvent(int eventId);
	void clearPort(void);
};

#endif
