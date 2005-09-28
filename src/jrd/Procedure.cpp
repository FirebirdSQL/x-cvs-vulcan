#include <stdio.h>
#include "fbdev.h"
#include "jrd.h"
#include "../jrd/all.h"
#include "../jrd/lck.h"
#include "../jrd/lck_proto.h"

#include "Procedure.h"
#include "ProcParam.h"
#include "ThreadData.h"
#include "ProcManager.h"
#include "../jrd/req.h"
#include "TempSpace.h"
#include "Request.h"
#include "blb.h"
#include "val.h"
#include "blr.h"
#include "blb_proto.h"
#include "par_proto.h"
#include "CompilerScratch.h"
#include "Format.h"
#include "Resource.h"

#define BLR_BYTE        *(csb->csb_running)++


//
// Constructor when id is known.
//
Procedure::Procedure(Database *dbb, int id)
{
	init();
	procId = id;
	procDatabase = dbb;
}


//
// Constructor when name and owner are known.
//

Procedure::Procedure(Database *dbb, const TEXT *name, const TEXT *owner, int id)
{
	init();
	procDatabase = dbb;
	procName = name;
	procOwner = owner;
	procId = id;
}

//
// Initializer
//

void Procedure::init(void)
{
	procNext = NULL;
	procSymbol = NULL;
	procInputParams = NULL;
	procOutputParams = NULL;
	procId = 0;
	procFlags = 0;
	procInputCount = 0;
	procOutputCount = 0;
	procInputMsg = NULL;
	procOutputMsg = NULL;
	procInputFormat = NULL;
	procOutputFormat = NULL;
	procFormat = NULL;
	procRequest = NULL;	
	procUseCount = 0;		
	procUseCountInternal = 0;	
	procExistenceLock = NULL;	
	procAlterCount = 0;	
	procManager = NULL;
}
//
// Destructor
//

Procedure::~Procedure()
{
	if (procManager)
		{
		procManager->purgeDependencies(this);
		procManager->remove(this);
		}
		
	while (ProcParam *param = procOutputParams)
		{
		procOutputParams = param->findNext();
		delete param;
		}

	while (ProcParam *param = procInputParams)
		{
		procInputParams = param->findNext();
		delete param;
		}
	
	if (procRequest)
		{
		procRequest->release();
		//CMP_release(tdbb, procRequest);
		procRequest = NULL;
		}
}

//
//  Add an output parameter
//

void Procedure::setOutputParameter (ProcParam *parameter)
{
	ProcParam **ptr;

	for (ptr = &procOutputParams; *ptr; ptr = &(*ptr)->paramNext)
		;
	
	*ptr = parameter;	
}

//
// Add an input parameter
//

void Procedure::setInputParameter (ProcParam *parameter)
{
	ProcParam **ptr;
	
	for (ptr = &procInputParams; *ptr; ptr = &(*ptr)->paramNext)
		;
	
	*ptr = parameter;	
}

//
// Express an interest in a procedure
//

void Procedure::lockExistence(thread_db*  tdbb)
{
	//LCK_lock(tdbb, procExistenceLock, LCK_SR, TRUE);
}

//
// Deny interest in a procedure
//

void Procedure::releaseExistence(thread_db*  tdbb)
{
	//LCK_release(procExistenceLock);
}

//
//  Lookup an input parameter by id
//

ProcParam *Procedure::findInputParameter (int id)
{
	return findParameterById (procInputParams, id);
}

//
// Lookup an output parameter by id
//

ProcParam *Procedure::findOutputParameter (int id)
{
	return findParameterById (procOutputParams, id);
}

//
// Find a generic parameter by id
//

ProcParam *Procedure::findParameterById (ProcParam *param, int id)
{
	ProcParam *ret_param = NULL;

	for (; param; param = param->findNext())
		if (param->findId() == id)
			{
			ret_param = param;
			break;
			}
	return ret_param;
}

//
// lookup an output parameter by name
//

int Procedure::findOutputParameter (const TEXT * name)
{
	return	findParameter (procOutputParams, name);
}

//
// Lookup an input parameter by name
//

int Procedure::findInputParameter (const TEXT * name)
{
	return  findParameter (procInputParams, name);
}

//
//  Find a generic parameter by name
//

int Procedure::findParameter (ProcParam *param, const TEXT *name)
{
	int ret_val = -1;

	if (!param)
		return ret_val;

	for (; param; param = param->findNext())
		if (param->isNamed (name))
			{
			ret_val = param->findId();
			break;
			}
	return ret_val;
}


//
//  Define an inequality function for parameters
//

bool Procedure::operator != (Procedure *proc)
{
	return !(proc->findName() == procName &&
		proc->findSecurityClassName() == procSecurityClassName &&
		proc->findOwner() == procOwner &&
		proc->findInputCount() == procInputCount &&
		proc->findOutputCount() == procOutputCount);
}

//
// Identify internal dependencies.
//

void Procedure::setDependencies()
{
	/* Walk procedures and calculate internal dependencies */
	
	if (Request *request = findRequest() ) 
		for (Resource* resource = request->req_resources; resource; resource = resource->next)
			if (resource->type == Resource::rsc_procedure)
				{
				fb_assert(resource->procedure->findInternalUseCount() >= 0);
				resource->procedure->incrementInternalUseCount();				
				}
}

Lock* Procedure::getExistenceLock(thread_db* tdbb)
{
	if (procExistenceLock)
		return procExistenceLock;

	Database *database = tdbb->tdbb_database;
	procExistenceLock = FB_NEW_RPT(*database->dbb_permanent, 0) Lock;
	procExistenceLock->lck_parent = database->dbb_lock;
	procExistenceLock->lck_dbb = database;
	procExistenceLock->lck_key.lck_long = findId();
	procExistenceLock->lck_length = sizeof(procExistenceLock->lck_key.lck_long);
	procExistenceLock->lck_type = LCK_prc_exist;
	procExistenceLock->lck_owner_handle = LCK_get_owner_handle(tdbb, LCK_prc_exist);
	procExistenceLock->lck_object = this;
	procExistenceLock->lck_ast = blockingAst;

	LCK_lock(tdbb, procExistenceLock, LCK_SR, TRUE);

	return procExistenceLock;
}

int Procedure::blockingAst(void* object)
{
	((Procedure*) object)->blockingAst();
	
	return 0;
}

/**************************************
 *
 *      b l o c k i n g _ a s t _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *      Someone is trying to drop a proceedure. If there
 *      are outstanding interests in the existence of
 *      the relation then just mark as blocking and return.
 *      Otherwise, mark the procedure block as questionable
 *      and release the procedure existence lock.
 *
 **************************************/

void Procedure::blockingAst(void)
{
	Database *database = procExistenceLock->lck_dbb;
	ISC_STATUS_ARRAY ast_status;
	ThreadData threadData (ast_status, database);
	LCK_release(procExistenceLock);
	addFlags(PRC_obsolete);

}

void Procedure::parseBlr(thread_db* tdbb, const bid *blobId)
{
	DBB dbb = tdbb->tdbb_database;
	JrdMemoryPool *old_pool = tdbb->tdbb_default;
	tdbb->tdbb_default = JrdMemoryPool::createPool(dbb);
	CompilerScratch csb(*tdbb->tdbb_default, 5);
	
	//parse_procedure_blr(tdbb, procedure, (SLONG*)&P.RDB$PROCEDURE_BLR, &csb);
	blb* blob = BLB_open(tdbb, dbb->dbb_sys_trans, blobId);
	const SLONG length = blob->blb_length + 10;
	
	TempSpace temp(length);
	BLB_get_data(tdbb, blob, temp.space, length);
	csb.csb_blr = temp.space;
	//par_messages(tdbb, temp.space, length, procedure, csb);
	parseMessages(tdbb, temp.space, length, &csb);
	Request* req = NULL;
	CompilerScratch* csbPtr = &csb;
	jrd_nod* node = PAR_blr(tdbb, NULL, temp.space, NULL, &csbPtr, &req, FALSE, 0);
	setRequest (req);
	//procRequest->req_procedure = procedure;

	for (int i = 0; i < csb.csb_rpt.getCount(); i++)
		{
		JRD_NOD node = csb.csb_rpt[i].csb_message;
		
		if (node)
			if ((IPTR) node->nod_arg[e_msg_number] == 0)
				procInputMsg = node; //setInputMsg (node);
			else if ((IPTR) node->nod_arg[e_msg_number] == 1)
				procOutputMsg = node; //setOutputMsg (node);
		}

	tdbb->tdbb_default = old_pool;
}

void Procedure::setRequest(Request* request)
{
	if (procRequest = request)
		request->req_procedure = this;
}

bool Procedure::parseMessages(thread_db* tdbb, const UCHAR* blr, int blrLength, CompilerScratch* csb)
{
	csb->csb_running = blr;
	const SSHORT version = BLR_BYTE;
	
	if (version != blr_version4 && version != blr_version5)
		return FALSE;

	if (BLR_BYTE != blr_begin)
		return FALSE;

	//SET_TDBB(tdbb);

	while (BLR_BYTE == blr_message) 
		{
		const USHORT msg_number = BLR_BYTE;
		USHORT count = BLR_BYTE;
		count += (BLR_BYTE) << 8;
		Format* format = Format::newFmt(*tdbb->tdbb_default, count);
		//format->fmt_count = count;
		// CVC: This offset should be protected against 32K overflow in the future
		USHORT offset = 0;
		
		for (Format::fmt_desc_iterator desc = format->fmt_desc.begin(); count; --count, ++desc)
			{
			const USHORT align = PAR_desc(csb, &*desc);
			if (align)
				offset = FB_ALIGN(offset, align);
			desc->dsc_address = (UCHAR *) (long) offset;
			offset += desc->dsc_length;
			}
			
		format->fmt_length = offset;

		if (msg_number == 0)
			procInputFormat = format; //setInputFormat (format);
		else if (msg_number == 1)
			procOutputFormat = format; //setOutputFormat (format);
		else
			delete format;
		}

	return TRUE;
}

void Procedure::setBlrBlobId(const void* blobId)
{
	memcpy(&procBlobId, blobId, sizeof(procBlobId));
}
