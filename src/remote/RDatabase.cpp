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
#include "common.h"
#include "ibase.h"
#include "RDatabase.h"
#include "remote.h"
#include "Sync.h"


RDatabase::RDatabase(Port *port)
{
	rdb_id = 0;
	rdb_flags = 0;
	rdb_handle = NULL_HANDLE;
	rdb_port = port;
	rdb_transactions = NULL;
	rdb_requests = NULL;
	rdb_events = NULL;
	rdb_sql_requests = NULL;
	rdb_status_vector = NULL;
	rdb_packet.zap();
}

RDatabase::~RDatabase(void)
{
	clearObjects();
	
	if (rdb_port)
		rdb_port->clearContext();
}

RTransaction* RDatabase::createTransactionId(OBJCT objectId)
{
	RTransaction* transaction = new RTransaction (this);
	transaction->rtr_id = objectId;
	Sync sync (&syncObject, "");
	
	sync.lock (Exclusive);
	transaction->rtr_next = rdb_transactions;
	rdb_transactions = transaction;
	sync.unlock();
	
	rdb_port->setObject (transaction, objectId);

	return transaction;
}

RTransaction* RDatabase::createTransactionHandle(isc_tr_handle transactionHandle)
{
	RTransaction* transaction = new RTransaction (this);
	transaction->rtr_handle = transactionHandle;
	transaction->rtr_id = rdb_port->getObjectId(transaction);
	Sync sync (&syncObject, "RDatabase::createTransaction");
	
	sync.lock (Exclusive);
	transaction->rtr_next = rdb_transactions;
	rdb_transactions = transaction;
	
	return transaction;
}

void RDatabase::releaseTransaction(RTransaction* transaction)
{
	Sync sync (&syncObject, "RDatabase::releaseTransaction");
	sync.lock (Exclusive);
	
	for (RTransaction **ptr = &rdb_transactions; *ptr; ptr = &(*ptr)->rtr_next)
		if (*ptr == transaction)
			{
			*ptr = transaction->rtr_next;
			break;
			}
}

void RDatabase::releaseRequest(RRequest* request)
{
	Sync sync (&syncObject, "RDatabase::releaseRequest");
	sync.lock (Exclusive);
	
	for (RRequest **ptr = &rdb_requests; *ptr; ptr = &(*ptr)->rrq_next)
		if (*ptr == request)
			{
			*ptr = request->rrq_next;
			break;
			}
}

void RDatabase::releaseStatement(RStatement* statement)
{
	Sync sync (&syncObject, "RDatabase::releaseStatement");
	sync.lock (Exclusive);
	
	for (RStatement **ptr = &rdb_sql_requests; *ptr; ptr = &(*ptr)->rsr_next)
		if (*ptr == statement)
			{
			*ptr = statement->rsr_next;
			break;
			}
}

void RDatabase::releaseEvent(REvent* event)
{
	Sync sync (&syncObject, "RDatabase::releaseEvent");
	sync.lock (Exclusive);
	
	for (REvent **ptr = &rdb_events; *ptr; ptr = &(*ptr)->rvnt_next)
		if (*ptr == event)
			{
			*ptr = event->rvnt_next;
			break;
			}
}

void RDatabase::clearObjects(void)
{
	if (rdb_port)
		rdb_port->clearStatement();
	
	while (rdb_transactions)
		delete rdb_transactions;
		
	while (rdb_requests)
		delete rdb_requests;
		
	while (rdb_sql_requests)
		delete rdb_sql_requests;
	
	while (rdb_events)
		delete rdb_events;
}

RRequest* RDatabase::createRequest(int numberMessages)
{
	RRequest *request = new RRequest (this, numberMessages);
	Sync sync (&syncObject, "RDatabase::createRequest");
	sync.lock (Exclusive);
	request->rrq_next = rdb_requests;
	rdb_requests = request;
	
	return request;
}


REvent* RDatabase::createEvent(void)
{
	REvent *object = new REvent (this);
	Sync sync (&syncObject, "RDatabase::createEvent");
	sync.lock (Exclusive);
	object->rvnt_next = rdb_events;
	rdb_events = object;
	
	return object;
}

RStatement* RDatabase::createStatement(void)
{
	RStatement *object = new RStatement (this);
	Sync sync (&syncObject, "RDatabase::createStatement");
	sync.lock (Exclusive);
	object->rsr_next = rdb_sql_requests;
	rdb_sql_requests = object;
	
	return object;
}

REvent* RDatabase::findEvent(int eventId)
{
	for (REvent *event = rdb_events; event; event = event->rvnt_next)
		if (event->rvnt_rid == eventId)
			return event;
	
	return NULL;
}

void RDatabase::clearPort(void)
{
	rdb_port = NULL;
}
