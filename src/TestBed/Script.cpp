// Script.cpp: implementation of the Script class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "Script.h"
#include "Editor.h"
#include "ScriptWindow.h"
#include "ScriptError.h"
#include "ActionConnect.h"
#include "ActionStatement.h"
#include "ActionRepeat.h"
#include "ActionWhile.h"
#include "ActionEnd.h"
#include "ActionNext.h"
#include "ActionScript.h"
#include "ActionThread.h"
#include "ActionWait.h"
#include "ActionExecute.h"
#include "ActionTimer.h"
#include "ActionPause.h"
#include "Context.h"
#include "SQLException.h"
#include "Stat.h"

#define WHITE	1
#define LETTER	2
#define DIGIT	4
#define PUNCT	8
#define EOL		16
#define QUOTE	32
#define ALPHA	(LETTER | DIGIT)

static char charTable [256];
static int initCharTable();
static int foo = initCharTable();

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


Script::Script (const char *scriptName, ApplicationObject *parent)
		: ApplicationObject (scriptName, parent, "Script", false)
{
	init();
}

Script::~Script()
{
	clear();
}

Script::Script()
{
	init();
}

void Script::init()
{
	terminal = true;
	extensible = false;
	compiled = false;
	//actions = NULL;
	deletable = true;
}

int initCharTable()
{
	for (int n = 'a'; n < 'z'; ++n)
		charTable [n] |= LETTER;

	for (n = 'A'; n < 'Z'; ++n)
		charTable [n] |= LETTER;

	for (n = '0'; n < '9'; ++n)
		charTable [n] |= LETTER;

	for (const char *p = " \t\r\n"; *p; ++p)
		charTable [*p] |= WHITE;

	for (p = "\r\n"; *p; ++p)
		charTable [*p] |= EOL;

	for (p = "'\""; *p; ++p)
		charTable [*p] |= QUOTE;

	for (p = ":=,"; *p; ++p)
		charTable [*p] |= PUNCT;

	return 0;
}
	
bool Script::doubleClick(TreeNode * node)
{
	openEditWindow();

	return true;
}

void Script::update(const char * text)
{
	clear();
	body = text;
	markChanged();
}

CString Script::getText()
{
	return body;
}

CEditor* Script::createEditWindow()
{
	CRuntimeClass* pRuntimeClass = RUNTIME_CLASS( CScriptWindow );
	CScriptWindow *editor = (CScriptWindow*) pRuntimeClass->CreateObject();
	editor->LoadFrame (IDR_EDITWINDOW, WS_OVERLAPPEDWINDOW, NULL );
	editor->setObject (this, getFullName(), getText());

	return editor;
}

void Script::execute()
{
	ApplicationObject *top;

	for (top = this; top->parent; top = top->parent)
		;

	top->update();
	Action *actions = NULL;
	Stat stat;

	try
		{
		Context context;
		actions = compile (&context);
		CTime start = CTime::GetCurrentTime();
		eval(actions, &stat);
		CTime finish = CTime::GetCurrentTime();
		CTimeSpan span = finish - start;
		CString msg;
		msg.Format ("Elapsed time: %d seconds", span.GetTotalSeconds());
		AfxMessageBox (msg);
		}
	catch (ScriptError& error)
		{
		AfxMessageBox (error.getText());
		}
	catch (SQLException& exception)
		{
		AfxMessageBox (exception.getText());
		}

	if (actions)
		{
		fini (actions);
		actions->release();
		actions = NULL;
		}
}

bool Script::skipWhite()
{
	eol = false;

	for (char c; (c = *text) && (charTable [c] & WHITE); ++text)
		if (charTable [c] & EOL)
			eol = true;

	if (!c)
		eol = true;

	return !eol;
}

bool Script::getToken()
{
	if (!skipWhite())
		return false;

	const char *start = text++;
	char c = *start;
    char type = charTable [*start];

	if (type & QUOTE)
		{
		while (*text && *text++ != c)
			;
		token = CString (start + 1, text - start - 2);
		return true;
		}

	if (!(type & PUNCT))
		for (; *text && !(charTable [*text] & (WHITE|PUNCT)); ++text)
			;

	token = CString (start, text - start);

	return true;
}

void Script::clear()
{
	compiled = false;
	/***
	for (Action *action; action = actions;)
		{
		actions = action->next;
		delete action;
		}
	***/
}

/*
 *	General grammar is:
 *
 *		[thread:] verb [name] [arg=value [, arg=value]]
 *
 */

Action* Script::parse()
{
	//clear();
	text = body;
	Action *actions = NULL;
	Action **ptr = &actions;

	for (;;)
		{
		// Find start of command.  If we run out of characters, we're done

		skipWhite ();
		if (!*text)
			break;
		getToken();
		// Pick up name.  Or maybe thread name.
		CString name = token;
		CString thread;
		const char *backup = text;
		getToken();

		// If this is a thread reference

		if (match (":"))
			{
			thread = name;
			name = token;
			}
		else
			text = backup;

		// Get action object and link it into the world

		Action *action = NULL;
		if (name.CompareNoCase ("connect") == 0)
			action = new ActionConnect;
		else if (name.CompareNoCase ("genHtml") == 0)
			action = new ActionStatement (GenHtml);
		else if (name.CompareNoCase ("commit") == 0)
			action = new ActionStatement (Commit);
		else if (name.CompareNoCase ("rollback") == 0)
			action = new ActionStatement (Rollback);
		else if (name.CompareNoCase ("update") == 0)
			action = new ActionStatement (Update);
		else if (name.CompareNoCase ("select") == 0)
			action = new ActionStatement (Select);
		else if (name.CompareNoCase ("disconnect") == 0)
			action = new ActionStatement (Disconnect);
		else if (name.CompareNoCase ("repeat") == 0)
			action = new ActionRepeat;
		else if (name.CompareNoCase ("while") == 0)
			action = new ActionWhile;
		else if (name.CompareNoCase ("end") == 0)
			action = new ActionEnd;
		else if (name.CompareNoCase ("next") == 0)
			action = new ActionNext;
		else if (name.CompareNoCase ("thread") == 0)
			action = new ActionThread;
		else if (name.CompareNoCase ("wait") == 0)
			action = new ActionWait;
		else if (name.CompareNoCase ("timer") == 0)
			action = new ActionTimer;
		else if (name.CompareNoCase ("pause") == 0)
			action = new ActionPause;
		else
			{
			Script *script = findScript (this, name);
			if (!script)
				throw ScriptError ("can't find script \"%s\"", (const char*) name);
			action = new ActionScript (script);
			}

		if (!thread.IsEmpty())
			action = new ActionExecute (thread, action);

		*ptr = action;
		ptr = &action->next;
		getArguments (action);
		}

	return actions;
}

bool Script::match(const char * string)
{
	if (token.CompareNoCase (string))
		return false;

	getToken();

	return true;
}

void Script::update()
{
	if (editor && editor->isChanged())
		{
		update (editor->getText());
		editor->clearChange();
		}
}

void Script::getArguments(Action * action)
{
	// Get action name (or may first argument)

	if (!getToken())
		return;

	for (bool first = true;; first = false)
		{
		CString argName = token;

		// If there isn't another token, there are no arguments

		if (!getToken())
			{
			if (first)
				{
				action->setName (argName);
				return;
				}
			throw ScriptError ("expected argument name");
			}

		if (!match ("="))
			if (first)
				{
				action->setName (argName);
				continue;
				}
			else
				throw ScriptError ("expected '=' in argument list");

		action->addArgument (argName, token);

		if (!getToken())
			break;

		if (!match (","))
			throw ScriptError ("expected comma in argument list");
		}
}

void Script::eval(Action *actions, Stat *stat)
{
	for (Action *action = actions; action;)
		action = action->eval (stat);
}

Action* Script::compile(Context *context)
{
	Action *actions = parse();
	Action *mark = context->mark();

	for (Action *action = actions; action;)
		action = action->compile (context);

	context->revert (mark);

	return actions;
}

void Script::fini (Action *actions)
{
	for (Action *action = actions; action; action = action->next)
		action->fini();
}

Script* Script::findScript(ApplicationObject * object, CString scriptName)
{
	if (name == scriptName)
		return this;

	if (parent && parent != object)
		return parent->findScript (this, scriptName);

	return NULL;
}
