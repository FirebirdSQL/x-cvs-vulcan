/*
 *	PROGRAM:		Client/Server Common Code
 *	MODULE:			semaphore.h
 *	DESCRIPTION:	Semaphore lock
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#ifdef WIN_NT

#include <windows.h>
#include <limits.h>

namespace firebird {

class Semaphore {
private:
	HANDLE hSemaphore;
public:
	Semaphore() { 
		hSemaphore = CreateSemaphore(NULL, 0 /*initial count*/, 
			INT_MAX, NULL); 
	}
	~Semaphore() { CloseHandle(hSemaphore); }	

	bool tryEnter(int seconds = 0) {
		DWORD result = WaitForSingleObject(
			hSemaphore, seconds >= 0 ? seconds * 1000 : INFINITE);
		if (result == WAIT_FAILED)
			system_call_failed::raise();
		return result != WAIT_TIMEOUT;
	}

	void release(SLONG count = 1) {
		if (!ReleaseSemaphore(hSemaphore, count, NULL))
			system_call_failed::raise();
	}
};


}

#else

#ifdef MULTI_THREAD

#include <semaphore.h>
#include <errno.h>

namespace firebird {

class Semaphore {
private:
	sem_t sem;
	bool  init;
public:
	Semaphore() : init(false) {
		if (sem_init(&sem, 0, 0) == -1) {
			gds__log("Error on semaphore.h: constructor");
			system_call_failed::raise();
		}
		init = true;
	}
	~Semaphore() {
		fb_assert(init == true);
		if (sem_destroy(&sem) == -1) {
			gds__log("Error on semaphore.h: destructor");
			system_call_failed::raise();
		}
		init = false;

	}
	bool tryEnter(int seconds = 0) {
		fb_assert(init == true);
		if (seconds == 0) {
			if (sem_trywait(&sem) != -1) 
				return true;
			if (errno == EAGAIN) 
				return false;
		} else if (seconds < 0) {
			if (sem_wait(&sem) != -1)
				return false;
		} else {
			struct timespec timeout;
			timeout.tv_sec = time(NULL) + seconds;
			timeout.tv_nsec = 0;
			if (sem_timedwait(&sem, &timeout) == 0) 
				return true;
			if (errno == ETIMEDOUT) 
				return false;
		}
		char message[128];
		sprintf(message, "Error on semaphore.h: tryEnter errno=%d\n", errno);
		gds__log(message);
		system_call_failed::raise();
	}
	void enter() {
		fb_assert(init == true);
		if (sem_wait(&sem) == -1) {
			gds__log("Error on semaphore.h: enter");
			system_call_failed::raise();
		}
	}
	void release(SLONG count = 1) {
		fb_assert(init == true);
		for (int i = 0; i < count; i++)
			if (sem_post(&sem) == -1) {
				gds__log("Error on semaphore.h: release");
				system_call_failed::raise();
			}
	}
};

}

#endif /*MULTI_THREAD*/

#endif /*!WIN_NT*/

#endif

