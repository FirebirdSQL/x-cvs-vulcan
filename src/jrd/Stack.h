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


#ifndef __STACK_H
#define __STACK_H

#define FOR_STACK(type,child,stack) {for (Stack *pos=(stack)->getTop();pos;){ type child = (type) (stack)->getPrior (&pos);

#ifndef END_FOR
#define END_FOR						 }}
#endif

class Stack {
    public:
	    bool isMember (void *object);
	    bool equal (Stack *stack);
	    bool insertOrdered (void *object);
	    void clear();
	    bool isMark (void *mark);
	    int count();
	    virtual  ~Stack();
	    bool isEmpty();
	    void* peek();
	    void pop (void *mark);
	    void* mark();

	Stack();

	void		push (void *object);
	void		*pop ();
	Stack		*getTop();
	void		*getPrior (Stack **);

	protected:

	Stack		*prior;
	void		*object;
    };

#endif

