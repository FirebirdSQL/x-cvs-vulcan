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

#include "../common/classes/vector.h"
#include "../common/classes/array.h"

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

namespace firebird {

	template <typename Object, size_t Capacity = 16>
		class Stack : public AutoStorage
	{
	private:
		Stack<Object, Capacity>(Stack<Object, Capacity>&);	// not implemented

		class Entry : public Vector<Object, Capacity> {
		private:
			typedef Vector<Object, Capacity> inherited;
		public:
			Entry* next;

			Entry(Object e, Entry* stk) 
				: inherited(), next(stk) 
			{
				add(e);
			}
			
			Entry(Entry* stk) : inherited(), next(stk) { }

			~Entry() 
			{
				delete next;
			}
			
			Entry* push(Object e, MemoryPool& p)
			{
				if (inherited::getCount() < this->getCapacity())
				{
					add(e);
					return this;
				}
				Entry* newEntry = FB_NEW(p) Entry(e, this);
				return newEntry;
			}

			Object pop()
			{
				fb_assert(inherited::getCount() > 0);
				return this->data[--this->count];
			}

			Object getObject(size_t pos) const
			{
				return this->data[pos];
			}

			void split(size_t elem, Entry* target) 
			{
				fb_assert(elem > 0 && elem < this->count);
				fb_assert(target->count == 0);
				target->count = this->count - elem;
				memcpy(target->data, &this->data[elem], target->count * sizeof(Object));
				this->count = elem;
			}

			Entry* dup(MemoryPool& p)
			{
				Entry* rc = FB_NEW(p) Entry(next ? next->dup(p) : 0);
				rc->join(*next);
				return rc;
			}

			bool hasMore(int value) const
			{
				fb_assert(value >= 0);
				
				if ((size_t) value <= inherited::getCount())
					return true;

				for (const Entry* stk = this; stk && value > 0; stk = stk->next)
					value -= stk->getCount();

				return (value <= 0);
			}

		}; // class Entry

		Entry* stk;

	public:
		explicit Stack<Object, Capacity>(MemoryPool& p) 
			: AutoStorage(p), stk(0) { }

		Stack<Object, Capacity>() : AutoStorage(), stk(0) { }

		~Stack<Object, Capacity>()
		{
			delete stk;
		}

		void push(Object e)
		{
			stk = stk ? stk->push(e, getPool()) 
					  : FB_NEW(getPool()) Entry(e, 0);
		}

		Object pop()
		{
			fb_assert(stk);
			Object tmp = stk->pop();
			if (!stk->getCount()) {
				Entry *oldEntry = stk;
				stk = stk->next;
				oldEntry->next = 0;
				delete oldEntry;
			}
			return tmp;
		}

	private:
		// disable use of default operator=
		Stack<Object, Capacity>& operator= (const Stack<Object, Capacity>& s);

	public:
		void takeOwnership (Stack<Object, Capacity>& s) {
			fb_assert(&getPool() == &s.getPool());
			delete stk;
			stk = s.stk;
			s.stk = 0;
		}

		class iterator;
		friend class iterator;

		class iterator {
		private:
			// friend definition here is required to implement
			// Merge/Split pair of functions
			friend class firebird::Stack<Object, Capacity>;
			const Entry* stk;
			size_t elem;

		public:
			explicit iterator(Stack<Object, Capacity>& s)
				: stk(s.stk), elem(stk ? stk->getCount() : 0) { }
			iterator(const iterator& i) 
				: stk(i.stk), elem(i.elem) { }
			iterator() : stk(0), elem(0) { }

			iterator& operator++()
			{
				fb_assert(stk);
				fb_assert(elem);
				if (--elem == 0) {
					if ((stk = stk->next)) 
					{
						elem = stk->getCount();
					}
				}
				return *this;
			}

			bool hasMore(int value) const
			{
				fb_assert(value >= 0);

				if (elem)
				{
					if ((size_t) value < elem)
						return true;
				
					value -= elem - 1;
				}

				if (stk && stk->next)
					return stk->next->hasMore(value);
				else
					return false;
			}

			bool hasData() const
			{
				return stk;
			}

			bool isEmpty() const
			{
				return !stk;
			}

			Object object() const
			{
				fb_assert(stk);
				return stk->getObject(elem - 1);
			}

			bool operator== (const iterator &i) const
			{
				return (stk == i.stk) && (elem == i.elem);
			}

			bool operator!= (const iterator &i) const
			{
				return !(*this == i);
			}

			bool operator== (const Stack<Object, Capacity>& s) const
			{
			return (this->stk == s.stk) &&
				   (s.stk ? this->elem == s.stk->getCount() : true);
			}

			bool operator!= (const Stack<Object, Capacity>& s) const
			{
				return !(*this == s);
			}

			iterator& operator= (iterator& i) {
				stk = i.stk;
				elem = i.elem;
				return *this;
			}

			iterator& operator= (Stack<Object, Capacity>& s) {
				stk = s.stk;
				elem = stk ? stk->getCount() : 0;
				return *this;
			}
			
			//friend void class Stack<Object, Capacity>::clear (const iterator &mark);
		}; // iterator

		class const_iterator;
		friend class const_iterator;
		
		class const_iterator {
		private:
			friend class firebird::Stack<Object, Capacity>;
			const Entry* stk;
			size_t elem;

		public:
			explicit const_iterator(const Stack<Object, Capacity>& s)
				: stk(s.stk), elem(stk ? stk->getCount() : 0) { }
			const_iterator(const iterator& i)
				: stk(i.stk), elem(i.elem) { }
			const_iterator(const const_iterator& i)
				: stk(i.stk), elem(i.elem) { }
			const_iterator() : stk(0), elem(0) { }

			const_iterator& operator++()
			{
				fb_assert(stk);
				fb_assert(elem);
				if (--elem == 0) {
					if ((stk = stk->next))
					{
						elem = stk->getCount();
					}
				}
				return *this;
			}

			bool hasMore(int value) const
			{
				fb_assert(value >= 0);

				if (elem)
				{
					if (value < elem)
						return true;

					value -= elem - 1;
				}

				if (stk && stk->next)
					return stk->next->hasMore(value);
				else
					return false;
			}

			bool hasData() const
			{
				return stk;
			}

			bool isEmpty() const
			{
				return !stk;
			}

			const Object object() const
			{
				fb_assert(stk);
				return stk->getObject(elem - 1);
			}

			bool operator== (const iterator &i) const
			{
				return (stk == i.stk) && (elem == i.elem);
			}

			bool operator== (const const_iterator &i) const
			{
				return (stk == i.stk) && (elem == i.elem);
			}

			bool operator!= (const iterator &i) const
			{
				return !(*this == i);
			}

			bool operator!= (const const_iterator &i) const
			{
				return !(*this == i);
			}

			bool operator== (const Stack<Object, Capacity>& s) const
			{
			return (this->stk == s.stk) &&
				   (s.stk ? this->elem == s.stk->getCount() : true);
			}

			bool operator!= (const Stack<Object, Capacity>& s) const
			{
				return !(*this == s);
			}

			const_iterator& operator= (const iterator& i) {
				stk = i.stk;
				elem = i.elem;
				return *this;
			}

			const_iterator& operator= (const const_iterator& i) {
				stk = i.stk;
				elem = i.elem;
				return *this;
			}

			const_iterator& operator= (const Stack<Object, Capacity>& s) {
				stk = s.stk;
				elem = stk ? stk->getCount() : 0;
				return *this;
			}
			
			//friend const const_iterator class Stack<Object, Capacity>::merge(Stack<Object, Capacity>& s);
			//void class Stack<Object, Capacity>::split (const const_iterator &mark, Stack<Object, Capacity>& s);
		}; // const_iterator


		// Merge stack "s" to the end of current one.
		// Returns - iterator to Split stacks again.
		// This iterator will be used as "bookmark" for Split later.
		const const_iterator merge(Stack<Object, Capacity>& s)
		{
			fb_assert(&getPool() == &s.getPool());
			const const_iterator rc(s);
			Entry **e = &stk;
			while (*e) 
			{
				e = &((*e)->next);
			}
			*e = s.stk;
			s.stk = 0;
			return rc;
		}

		// Split stacks at mark
		void split (const const_iterator &mark, Stack<Object, Capacity>& s)
		{
			fb_assert(&getPool() == &s.getPool());
			fb_assert(!s.stk);

			// if empty stack was merged, there is nothing to do
			if (!mark.stk)
			{
				return;
			}

			// find entry to Split
			Entry **toSplit = &stk;
			while (*toSplit != mark.stk)
			{
				fb_assert(*toSplit);
				toSplit = &((*toSplit)->next);
			}

			// Determine whether some new elements were added
			// to this stack. Depended on this we must
			// Split on entries boundary or cut one entry to halfs.
			fb_assert((*toSplit)->getCount() >= mark.elem);
			if ((*toSplit)->getCount() == mark.elem)
			{
				s.stk = *toSplit;
				*toSplit = 0;
			}
			else {
				Entry* newEntry = FB_NEW(getPool()) Entry(0);
				(*toSplit)->split(mark.elem, newEntry);
				s.stk = *toSplit;
				*toSplit = newEntry;
			}
		}

		// clear stacks until mark
		void clear (const iterator &mark)
		{
			// for empty mark just clear all stack
			if (!mark.stk)
			{
				clear();
				return;
			}

			// find entry to clear
			while (stk != mark.stk)
			{
				if (!stk)
				{
					return;
				}
				Entry *tmp = stk->next;
				stk->next = 0;
				delete stk;
				stk = tmp;
			}

			// remove extra elements from Entry
			fb_assert(stk->getCount() >= mark.elem);
			if (mark.elem == 0)
			{
				Entry *tmp = stk->next;
				stk->next = 0;
				delete stk;
				stk = tmp;
			}
			else {
				stk->shrink(mark.elem);
			}
		}

		size_t getCount() const {
			size_t rc = 0;
			for (Entry* entry = stk; entry; entry = entry->next)
			{
				rc += entry->getCount();
			}
			return rc;
		}

		bool hasMore(int value) const
		{
			fb_assert(value >= 0);
			return (stk && stk->hasMore(value));
		}

		// returns topmost element on the stack
		Object object() const
		{
			fb_assert(stk);
			return stk->getObject(stk->getCount() - 1);
		}

		// returns true if stack is not empty
		bool hasData() const
		{
			return stk;
		}

		// returns true if stack is empty
		bool isEmpty() const
		{
			return !stk;
		}

		bool operator== (const iterator &i) const
		{
			return i == *this;
		}

		bool operator!= (const iterator &i) const
		{
			return !(i == *this);
		}

		bool operator== (const const_iterator &i) const
		{
			return i == *this;
		}

		bool operator!= (const const_iterator &i) const
		{
			return !(i == *this);
		}

		void assign(Stack<Object, Capacity>& v) {
			delete stk;
			stk = v.stk ? v.stk->dup(getPool()) : 0;
		}

		void clear()
		{
			delete stk;
			stk = 0;
		}

		MemoryPool& getPool() 
		{
			return AutoStorage::getPool();
		}
	}; // class Stack

} // namespace firebird


#endif

