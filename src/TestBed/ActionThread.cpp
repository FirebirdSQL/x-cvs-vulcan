// ActionThread.cpp: implementation of the ActionThread class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ActionThread.h"
#include "Context.h"
#include "SQLError.h"
#include "Sync.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ActionThread::ActionThread() : Action (actThread)
{
	firstAction = lastAction = NULL;
	running = false;
	stalled = false;
}

ActionThread::~ActionThread()
{
	thread.shutdown();
}

Action* ActionThread::compile(Context * context)
{
	context->push (this);

	return next;
}

Action* ActionThread::eval(Stat *stat)
{
	if (!running)
		{
		thread.start(run, this);
		while (!running)
			stallSync.sleep();
		}

	return next;
}

void ActionThread::fini()
{
	if (running)
		thread.shutdown();
}

void ActionThread::wait()
{
	while (running && !stalled)
		stallSync.sleep (1000);

	if (error)
		throw SQLError (sqlcode, errorText);
}

void ActionThread::queAction(Action * action, Stat *statistic)
{
	action->que = NULL;
	stat = statistic;
	Sync sync (&syncObject);
	sync.lock (Exclusive);

	if (lastAction)
		{
		lastAction->next = action;
		lastAction = action;
		}
	else
		firstAction = lastAction = action;

	thread.wake();
}

ULONG ActionThread::run(void *arg)
{
	return ((ActionThread*) arg)->runThread();
}

ULONG ActionThread::runThread()
{
	addRef();
	running = true;
	error = false;
	stallSync.wake();
	Sync sync (&syncObject);

	try
		{
		while (!thread.shutdownInProgress)
			{
			if (firstAction)
				{
				sync.lock (Exclusive);
				Action *action = firstAction;
				if (action)
					{
					if (!(firstAction = action->que))
						lastAction = NULL;
					sync.unlock();
					action->eval (stat);
					}
				else
					sync.unlock();
				}
			else
				{
				stalled = true;
				stallSync.wake();
				thread.sleep();
				stalled = false;
				}
			}
		}
	catch (SQLException& exception)
		{
		error = true;
		errorText = exception.getText();
		sqlcode = exception.getSqlcode();
		}
	catch (...)
		{
		running = false;
		error = true;
		errorText = "unknown exception";
		//throw;
		}

	running = false;
	release();

	return 0;
}
