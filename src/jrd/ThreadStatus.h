/*
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 */

//
//////////////////////////////////////////////////////////////////////

#ifndef _THREAD_STATUS_H
#define _THREAD_STATUS_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "tdbb.h"

class Transaction;
class Attachment;

class ThreadStatus
{
public:
	inline ThreadStatus (thread_db *tdbb, Transaction *transaction)
		{
		threadStuf = tdbb;
		priorTransaction = tdbb->tdbb_transaction;
		priorAttachment = tdbb->tdbb_attachment;
		tdbb->tdbb_transaction = transaction;
		tdbb->tdbb_attachment = transaction->tra_attachment;
		}
	
	inline ~ThreadStatus ()
		{
		threadStuff->tdbb_transaction = priorTransaction;
		threadStuff->tdbb_attachment = priorAttachment;
		}
		
	thread_db	*threadStuff;
	Transaction	*priorTransaction;
	Attachment	*priorAttachment;
};

#endif
