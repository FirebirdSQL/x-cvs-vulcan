/*
 *  
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
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */


#include "firebird.h"
#include "Stack.h"

#ifdef _DEBUG
static char THIS_FILE[]=__FILE__;
#endif


Stack::Stack ()
{
/**************************************
 *
 *		S t a c k
 *
 **************************************
 *
 * Functional description
 *		Constructor.
 *
 **************************************/

prior = NULL;
}

void *Stack::getPrior (Stack **ptr)
{
/**************************************
 *
 *		g e t P r i o r
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
Stack *node = *ptr;
*ptr = node->prior;

return node->object;
}

Stack *Stack::getTop ()
{
/**************************************
 *
 *		g e t T o p
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

return prior;
}

void *Stack::pop ()
{
/**************************************
 *
 *		p o p
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
Stack *node = prior;
void *object = node->object;
prior = node->prior;
node->prior = NULL;
delete node;

return object;
}


void Stack::push (void *object)
{
/**************************************
 *
 *		p u s h
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

Stack *node = new Stack;
node->object = object;
node->prior = prior;
prior = node;
}


void* Stack::mark()
{
	return prior;	
}

void Stack::pop(void * mark)
{
	while (prior && prior != mark)
		pop();	
}

void* Stack::peek()
{
	if (!prior)
		return NULL;

	return prior->object;
}

bool Stack::isEmpty()
{
	return prior == NULL;
}

Stack::~Stack()
{
	while (prior)
		pop();	
}

int Stack::count()
{
	int n = 0;

	for (Stack *node = prior; node; node = node->prior)
		++n;

	return n;
}

bool Stack::isMark(void * mark)
{
	return prior == mark;
}

void Stack::clear()
{
	while (prior)
		pop();	
}

bool Stack::insertOrdered(void *object)
{
	Stack **ptr = &prior;
	Stack *node;

	for (; node = *ptr; ptr = &node->prior)
		if (node->object <= object)
			{
			if (node->object == object)
				return false;
			break;
			}

	Stack *s = *ptr = new Stack;
	s->object = object;
	s->prior = node;

	return true;
}

bool Stack::equal(Stack *stack)
{
	Stack *s1 = prior;
	Stack *s2 = stack->prior;

	for (; s1 && s2; s1 = s1->prior, s2 = s2->prior)
		if (s1->object != s2->object)
			return false;

	if (s1 || s2)
		return false;

	return true;
}

bool Stack::isMember(void *object)
{
	for (Stack *node = prior; node; node = node->prior)
		if (node->object == object)
			return true;

	return false;
}
