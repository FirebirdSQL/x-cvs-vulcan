/*
 *	PROGRAM:		Virtual Data Manager
 *	MODULE:			LinkedList.h
 *	DESCRIPTION:	Generic Linked List
 *
 * copyright (c) 1997 - 2000 by James A. Starkey
 */
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


#ifndef __LINKEDLIST_H
#define __LINKEDLIST_H

#define FOR_OBJECTS(type,child,list) {for (LinkedList *pos=(list)->getHead();(list)->more (pos);){ type child = (type) (list)->getNext (&pos);
#define FOR_OBJECTS_BACKWARD(type,child,list) {for (LinkedList *pos=(list)->getTail();(list)->moreBackwards (pos);){ type child = (type) (list)->getPrior (&pos);
#define END_FOR						 }}

#ifndef NULL
#define NULL	0
#endif

START_NAMESPACE

class LinkedNode;

class LinkedList
    {
	public:
		void addressCheck (void *address);
		bool insertBefore (void *insertItem, void *item);

	LinkedList();
	~LinkedList();

	void		append (void *object);
	bool		appendUnique (void *object);
	void		clear();
	int			count ();
	bool		deleteItem (void* object);

	LinkedList	*getHead ();
	bool		more (LinkedList *node);
	void		*getNext(LinkedList **node);

	LinkedList	*getTail ();
	void		*getElement (int position);
	void		*getPrior(LinkedList**);
	bool		isEmpty();
	bool		isMember (void *object);
	
	bool		moreBackwards (LinkedList*);

	protected:
	
	LinkedNode		*next, *prior;
	//void			*object;
	};

class LinkedNode : public LinkedList {
	public:

	LinkedNode (void *object);
    ~LinkedNode();

	void			*object;
	};

inline LinkedList *LinkedList::getHead ()
	{
	return (LinkedList*) next;
	}

inline bool	LinkedList::more (LinkedList *node)
	{
	return (node != NULL);
	}

inline void	*LinkedList::getNext(LinkedList **node)
	{
	void *object = ((LinkedNode*)(*node))->object;
	*node = (*node)->next;

	return object;
	}

END_NAMESPACE

#endif

