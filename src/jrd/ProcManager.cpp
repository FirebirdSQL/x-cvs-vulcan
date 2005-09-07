#include "firebird.h"
#include "../jrd/all.h"
#include "../jrd/lck.h"
#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/gds_proto.h"
#include "ProcManager.h"
#include "JString.h"
#include "Procedure.h"
#include "../jrd/req.h"
#include "../jrd/cmp_proto.h"
#include "Stream.h"
#include "Sync.h"

ProcManager::ProcManager(Database * dbb)
{
	database = dbb;
	procList = NULL;
}



ProcManager::~ProcManager()
{
	for (Procedure *procedure; procedure = procList;)
		{
		procList = procedure->findNext();
		delete procedure;
		}
}



void	ProcManager::addProcedure (Procedure *procedure)
{
	Sync sync(&syncObject, "ProcManager::addProcedure");
	sync.lock(Exclusive);
	procedure->procManager = this;
	procedure->setNext (procList);
	procList = procedure;
}



void	ProcManager::dropProcedure (int id)
{
	Sync sync(&syncObject, "ProcManager::dropProcedure");
	sync.lock(Exclusive);
	Procedure *prev_procedure = procList;

	for (Procedure *procedure = findFirst(); procedure; procedure->findNext())
		{
		if (procedure->findId() == id)
			{
			prev_procedure->setNext(procedure->findNext());
			delete procedure;
			}
		else
			prev_procedure = procedure;
		}
}



void	ProcManager::dropProcedure (const TEXT * name)
{
	Sync sync(&syncObject, "ProcManager::dropProcedure(name)");
	sync.lock(Exclusive);

	for (Procedure **ptr = &procList, *procedure; procedure = *ptr; ptr = &procedure->procNext)
		if (procedure->isNamed (name))
			{
			*ptr = procedure->procNext;
			delete procedure;
			break;
			}
}



Procedure* ProcManager::findKnownProcedure (const TEXT *name)
{
	for (Procedure *procedure = findFirst(); procedure; procedure = procedure->findNext())
		if (procedure->isNamed(name))
			return procedure;
	
	return NULL;
	/***
	Procedure *procedure;
	Procedure *check_procedure = NULL;

	for (procedure = findFirst(); procedure; procedure = procedure->findNext())
		{
		if (procedure->isNamed(name) && procedure->checkActive (noscan))
			{
			if (!(procedure->findFlags() & PRC_check_existence)) 
				return procedure;

			check_procedure = procedure;
			check_procedure->lockExistence (tdbb);
			break;
			}

		if (check_procedure) 
			break;
		}

	if (check_procedure) 
		{
		check_procedure->clearFlags (PRC_check_existence);
		if (check_procedure != procedure)
			{
			check_procedure->releaseExistence(tdbb);
			check_procedure->addFlags(PRC_obsolete);
			}
		}
		
	return procedure;
	***/
}



Procedure * ProcManager::findKnownProcedure (thread_db* tdbb, int id)
{	
	for (Procedure *procedure = procList; procedure; procedure = procedure->procNext)
		if (procedure->procId == id)
			return procedure;

	return NULL;
}



Procedure * ProcManager::findProcedure (thread_db* tdbb, const TEXT *name, bool noscan)
{
	Sync sync(&syncObject, "ProcManager::findProcedure");
	sync.lock(Shared);
	Procedure *procedure = findKnownProcedure(name);

	if (procedure)
		{
		if (procedure->checkActive (noscan) && !(procedure->findFlags() & PRC_check_existence)) 
			return procedure;

		procedure->lockExistence (tdbb);
		procedure->clearFlags (PRC_check_existence);
		
		return procedure;
		}

	if (noscan)
		return NULL;

	sync.unlock();
	Procedure *newProcedure = MET_lookup_procedure(tdbb, name);
	sync.lock(Exclusive);
	
	if (procedure = findKnownProcedure(name))
		delete newProcedure;
		
	if (!newProcedure)
		return NULL;
		
	addProcedure (newProcedure);
	procedure = newProcedure;
	sync.unlock();
	newProcedure->parseBlr(tdbb, (bid*) &newProcedure->procBlobId);
	
	if (procedure->checkActive (noscan) && !(procedure->findFlags() & PRC_check_existence)) 
		return procedure;

	procedure->lockExistence (tdbb);
	procedure->clearFlags (PRC_check_existence);
	
	return procedure;
}



Procedure * ProcManager::findProcedure (thread_db* tdbb, int id)
{	
	Sync sync(&syncObject, "ProcManager::findProcedure");
	sync.lock(Shared);
	Procedure *procedure = findKnownProcedure (tdbb, id);
	
	if (!procedure || procedure->checkFlags(PRC_obsolete))
		return NULL;
	
	return procedure;
}



Procedure * ProcManager::findProcedure (thread_db* tdbb, JString name, bool noscan)
{
	return findProcedure (tdbb, (const TEXT*) name, noscan);
}


/***
Procedure * ProcManager::createProcedure ()
{	
	Procedure * proc = new Procedure ();
	proc->addFlags (PRC_create);
	proc->setNext (procList);
	procList = proc;
	return proc;
}

Procedure * ProcManager::createProcedure (const char * name, const char *owner)
{
	Procedure * proc = createProcedure ();
	proc->setName (name);
	proc->setOwner(owner);
	return proc;
}
***/

void ProcManager::validateCache (thread_db* tdbb)
{
	Procedure* procedure;
	
	for (procedure = findFirst(); procedure; procedure = procedure->findNext())
		procedure->setDependencies();
		
	/* Walk procedures again and check dependencies */
	
	for (procedure = findFirst(); procedure; procedure = procedure->findNext())
		{
		if (procedure->hasRequest() && 
			 procedure->findUseCount() < procedure->findInternalUseCount())
			{
			Stream stream;
			stream.format 
				("Procedure %d:%s is not properly counted (use count=%d, prc use=%d). Used by: \n", 
				procedure->findId(), (const char *)procedure->findName(), 
				procedure->findUseCount(), procedure->findInternalUseCount());

			Procedure* prc;
			for (prc = findFirst(); prc; prc = prc->findNext()) 
				{
				if (prc->hasRequest()) 
					{
					for (Resource* resource = prc->findRequest()->req_resources; resource;
						resource = resource->rsc_next)
						{
						if (resource->rsc_type == Resource::rsc_procedure)
							if (resource->rsc_prc == procedure) 
								stream.format ("%d:%s\n", 
									prc->findId(), (const TEXT *)prc->findName());
						}
					}
				}

			gds__log  (stream.getJString());
			}
		}		
	/* Fix back int_use_count */		
	for (procedure = findFirst(); procedure; procedure = procedure->findNext()) 
		procedure->setInternalUseCount(0);
}



bool ProcManager::clearCache(thread_db* tdbb, Procedure *proc)
{
	bool result = TRUE;
	Procedure *procedure = findFirst();

	/* Walk procedures and calculate internal dependencies */
	for (; procedure; procedure->findNext()) 
		{
		if ( procedure->hasRequest() &&
			!(procedure->checkFlags (PRC_obsolete))) 
			{
			procedure->setDependencies();
			}
		}
	
	/* Walk procedures again and adjust dependencies for procedures which will not be removed */
	for (procedure = findFirst(); procedure; procedure->findNext()) 
		{
		if (procedure->hasRequest() &&
				!(procedure->checkFlags (PRC_obsolete)) && 
				procedure->findUseCount() != procedure->findInternalUseCount() && 
				procedure != proc )
			{
			adjustDependencies (procedure);
			}
		}

	if (proc) 
		{
		result = proc->findUseCount() == proc->findInternalUseCount();
		if (proc->findRequest())
			adjustDependencies(proc);
		}
	
	/* Deallocate all used requests */		
	for (procedure = findFirst(); procedure; procedure->findNext()) 
		{
		if ( procedure->findRequest() && 
				!(procedure->checkFlags(PRC_obsolete)) && 
				procedure->findInternalUseCount() >= 0 &&  
				procedure->findUseCount() == procedure->findInternalUseCount() && 
				procedure != proc ) 
			{
			CMP_release(tdbb, procedure->findRequest());
			procedure->setRequest (NULL);
			LCK_release(procedure->findExistenceLock());
			procedure->setExistenceLock (NULL);
			procedure->setFlags (PRC_obsolete);
			}
		// Leave it in state 0 to avoid extra pass next time to clear it
		// Note: we need to adjust prc_int_use_count for all procedures
		// in cache because any of them may have been affected from
		// dependencies earlier. Even procedures that were not scanned yet !
		procedure->setInternalUseCount (0);
		}
	
	/* Remove deallocated procedures from cache */
	for (procedure = findFirst(); procedure; procedure->findNext()) 
		{
		if ((procedure->checkFlags (PRC_obsolete)) && (procedure != proc) ) 
			{
			procedure->clearFlags (PRC_being_altered); // Just a safety sake
			MET_remove_procedure(tdbb, procedure);			  
			}
		}

	return result;
}
		
void ProcManager::adjustDependencies(Procedure* procedure) 
{
	if (procedure->findInternalUseCount()==-1) 
		return;

	procedure->setInternalUseCount(-1); // Mark as undeletable
	
	Request *request = procedure->findRequest();
	if (request) 
		{
		for (Resource* resource = request->req_resources; resource;
		  resource = resource->rsc_next) 
			{
			if (resource->rsc_type == Resource::rsc_procedure) 
				{
				procedure = resource->rsc_prc;
				
				if (procedure->findInternalUseCount() == procedure->findUseCount())
					adjustDependencies (procedure); 
				}
			}
		}
}

	
bool ProcManager::procedureInUse (thread_db* tdbb, Procedure *proc)
{
	/* Walk procedures and calculate internal dependencies */

	Procedure *procedure = findFirst();
	for (; procedure; procedure->findNext())
		{
		if (procedure->hasRequest() &&
			 !(procedure->checkFlags (PRC_obsolete))) 
			{
			for (Resource* resource = procedure->findRequest()->req_resources; resource;
				 resource = resource->rsc_next)
				{
				if (resource->rsc_type == Resource::rsc_procedure)
					resource->rsc_prc->incrementInternalUseCount();				
				}
			}
		}

	/* Walk procedures again and adjust dependencies for procedures which will not be removed */

	for (procedure = findFirst(); procedure; procedure->findNext())
		{
		if (procedure->hasRequest() &&
			 !(procedure->checkFlags (PRC_obsolete))&& 
			 procedure->findUseCount() != procedure->findInternalUseCount() && 
			 procedure != proc )
			{
			adjustDependencies(procedure);
			}
		}
		
	const BOOLEAN result = proc->findUseCount() != proc->findInternalUseCount();
		
	/* Fix back int_use_count */
	for (procedure = findFirst(); procedure; procedure->findNext())
		{				
		procedure->setInternalUseCount(0);
		}
	
	return result;
}


Procedure* ProcManager::findObsoleteProcedure(thread_db* tdbb, int id)
{
	Sync sync(&syncObject, "ProcManager::findProcedure");
	sync.lock(Shared);
	
	return findKnownProcedure (tdbb, id);
}

void ProcManager::remove(Procedure* procedure)
{
	Sync sync(&syncObject, "ProcManager::remove");
	sync.lock(Exclusive);

	for (Procedure **ptr = &procList; *ptr; ptr = &(*ptr)->procNext)
		if (*ptr == procedure)
			{
			*ptr = procedure->procNext;
			//delete procedure;
			break;
			}
}
