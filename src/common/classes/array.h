/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		array.h
 *	DESCRIPTION:	dynamic array of simple elements
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * Created by: Alex Peshkov <peshkoff@mail.ru>
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */
 
#ifndef ARRAY_H
#define ARRAY_H

#include "../jrd/gdsassert.h"
#include <string.h>
#include "../common/classes/alloc.h"

namespace firebird {

// Permanent storage is used as base class for all objects,
// performing memory allocation in methods other than 
// constructors of this objects. Permanent means that pool,
// which will be later used for such allocations, must
// be explicitly passed in all constructors of such object.
class PermanentStorage {
private:
	MemoryPool& pool;
protected:
	explicit PermanentStorage(MemoryPool& p) : pool(p) { }
	MemoryPool& getPool() const { return pool; }
};

// Automatic storage is used as base class for objects,
// that may have constructors without explicit MemoryPool
// parameter. In this case AutoStorage sends AutoMemoryPool
// to PermanentStorage. To ensure this operation to be safe
// such trick possible only for local (on stack) variables.
class AutoStorage : public PermanentStorage {
public:
	static MemoryPool& getAutoMemoryPool() {
		MemoryPool* p = getDefaultMemoryPool();
		fb_assert(p);
		return *p; 
	}
protected:
	AutoStorage() : PermanentStorage(getAutoMemoryPool()) {
	}
	explicit AutoStorage(MemoryPool& p) : PermanentStorage(p) { }
};

// Static part of the array
template <typename T, size_t Capacity>
class InlineStorage : public AutoStorage {
public:
	explicit InlineStorage(MemoryPool& p) : AutoStorage(p) { }
	InlineStorage() : AutoStorage() { }
protected:
	T* getStorage() {
		return buffer;
	}
	size_t getStorageSize() const {
		return Capacity;
	}
private:
	T buffer[Capacity];
};

// Used when array doesn't have static part
template <typename T>
class EmptyStorage : public AutoStorage {
public:
	explicit EmptyStorage(MemoryPool& p) : AutoStorage(p) { }
	EmptyStorage() : AutoStorage() { }
protected:
	T* getStorage() { return NULL; }
	size_t getStorageSize() const { return 0; }
};

// Dynamic array of simple types
template <typename T, typename Storage = EmptyStorage<T> >
class Array : protected Storage {
public:
	explicit Array(MemoryPool& p) : 
		Storage(p), count(0), capacity(this->getStorageSize()), data(this->getStorage()) {}
		
	Array(MemoryPool& p, int InitialCapacity) : 
		Storage(p), count(0), capacity((int) this->getStorageSize()), data(this->getStorage())
		{
		ensureCapacity(InitialCapacity);
		}
		
	explicit Array(MemoryPool* p) : 
		Storage(*p), count(0), capacity((int) this->getStorageSize()), data(this->getStorage()) {}
		
	Array(MemoryPool* p, int InitialCapacity) : 
		Storage(*p), count(0), capacity((int) this->getStorageSize()), data(this->getStorage())
		{
		ensureCapacity(InitialCapacity);
		}
		
	Array() : count(0), 
		capacity((int) this->getStorageSize()), data(this->getStorage()) { }
		
	explicit Array(int InitialCapacity) : count(0),
		capacity((int) this->getStorageSize()), data(this->getStorage())
		{
		ensureCapacity(InitialCapacity);
		}
		
	~Array()
		{
		freeData();
		}
		
	void clear() 
		{ 
		count = 0; 
		}
		
protected:
	T& getElement(int index) const 
		{
  		fb_assert(index < count);
  		return data[index];
		}
	
	T& getElement(int index) 
		{
  		fb_assert(index < count);
  		return data[index];
		}
	
	void freeData()
		{
		if (data != this->getStorage())
			this->getPool().deallocate(data);
		}
		
public:
	Array<T, Storage>& operator =(const Array<T, Storage>& L) 
		{
		ensureCapacity(L.count);
		memcpy(data, L.data, sizeof(T) * L.count);
		count = L.count;
		return *this;
		}
		
	const T& operator[](int index) const 
		{
  		return getElement(index);
		}
		
	T& operator[](int index) 
		{
  		return getElement(index);
		}
		
	const T& front() const 
		{
  		fb_assert(count > 0);
		return *data;
		}
		
	const T& back() const 
		{
  		fb_assert(count > 0);
		return *(data + count - 1);
		}
		
	const T* begin() const 
		{ 
		return data; 
		}
	const T* end() const 
		{ 
		return data + count; 
		}
	T& front() 
		{
  		fb_assert(count > 0);
		return *data;
		}
		
	T& back() 
		{
  		fb_assert(count > 0);
		return *(data + count - 1);
		}
		
	T* begin() { return data; }
	T* end() { return data + count; }
	void insert(int index, const T& item) {
		fb_assert(index <= count);
		ensureCapacity(count + 1);
		memmove(data + index + 1, data + index, sizeof(T) * (count++ - index));
		data[index] = item;
	}
	void insert(size_t index, const Array<T, Storage>& L) {
		fb_assert(index <= count);
		ensureCapacity(count + L.count);
		memmove(data + index + L.count, data + index, sizeof(T) * (count - index));
		memcpy(data + index, L.data, L.count);
		count += L.count;
	}
	void insert(size_t index, const T* items, size_t itemsSize) {
		fb_assert(index <= count);
		ensureCapacity(count + itemsSize);
		memmove(data + index + itemsSize, data + index, sizeof(T) * (count - index));
		memcpy(data + index, items, sizeof(T) * itemsSize);
		count += itemsSize;
	}
	int add(const T& item) {
		ensureCapacity(count+1);
		data[count++] = item;
  		return count;
	};
	void remove(int index) {
  		fb_assert(index < count);
  		memmove(data + index, data + index + 1, sizeof(T) * (--count - index));
	}
	void removeRange(int from, int to) {
  		fb_assert(from <= to);
  		fb_assert(to <= count);
  		memmove(data + from, data + to, sizeof(T) * (to - from));
		count -= (to - from);
	}
	void removeCount(int index, int n) {
  		fb_assert(index + n <= count);
  		memmove(data + index, data + index + n, sizeof(T) * n);
		count -= n;
	}
	void remove(T* itr) {
		const int index = itr - begin();
  		fb_assert(index < count);
  		memmove(data + index, data + index + 1, sizeof(T) * (--count - index));
	}
	void shrink(int newCount) {
		fb_assert(newCount <= count);
		count = newCount;
	};
	// Grow size of our array and zero-initialize new items
	void grow(int newCount) {
		fb_assert(newCount >= count);
		ensureCapacity(newCount);
		memset(data + count, 0, sizeof(T) * (newCount - count));
		count = newCount;
	}
	void join(const Array<T, Storage>& L) {
		ensureCapacity(count + L.count);
		memcpy(data + count, L.data, sizeof(T) * L.count);
		count += L.count;
	}
	int getCount() const { return count; }
	int getCapacity() const { return capacity; }
	void push(const T& item) {
		add(item);
	}
	void push(const T* items, int itemsSize) {
		ensureCapacity(count + itemsSize);
		memcpy(data + count, items, sizeof(T) * itemsSize);
		count += itemsSize;
	}
	T pop() {
		fb_assert(count > 0);
		count--;
		return data[count];
	}
	// prepare array to be used as a buffer of capacity items
	T* getBuffer(int capacityL) {
		ensureCapacity(capacityL);
		count = capacityL;
		return data;
	}
	// clear array and release dinamically allocated memory
	void free() 
	{
		clear();
		freeData();
		capacity = this->getStorageSize();
		data = this->getStorage();
	}

protected:
	int count, capacity;
	T* data;
	void ensureCapacity(int newcapacity) {
		if (newcapacity > capacity) {
			if (newcapacity < capacity * 2) {
				newcapacity = capacity * 2;
			}
			T* newdata = reinterpret_cast<T*>
				(this->getPool().allocate(sizeof(T) * newcapacity
#ifdef DEBUG_GDS_ALLOC
		, 1, __FILE__, __LINE__
#endif
						));
			memcpy(newdata, data, sizeof(T) * count);
			freeData();
			data = newdata;
			capacity = newcapacity;
		}
	}
};


// Dynamic sorted array of simple objects
template <typename Value,
	typename Storage = EmptyStorage<Value>, 
	typename Key = Value, 
	typename KeyOfValue = DefaultKeyValue<Value>, 
	typename Cmp = DefaultComparator<Key> >
class SortedArray : public Array<Value, Storage> {
public:
	SortedArray(MemoryPool& p, size_t s) : Array<Value, Storage>(p, s) {}
	explicit SortedArray(MemoryPool& p) : Array<Value, Storage>(p) {}
	SortedArray(MemoryPool* p, size_t s) : Array<Value, Storage>(p, s) {}
	explicit SortedArray(MemoryPool* p) : Array<Value, Storage>(p) {}
	explicit SortedArray(int s) : Array<Value, Storage>(s) {}
	SortedArray() : Array<Value, Storage>() {}
	bool find(const Key& item, int& pos) {
		int highBound = this->count, lowBound = 0;
		while (highBound > lowBound) {
			const int temp = (highBound + lowBound) >> 1;
			if (Cmp::greaterThan(item, KeyOfValue::generate(this, this->data[temp])))
				lowBound = temp + 1;
			else
				highBound = temp;
		}
		pos = lowBound;
		return highBound != this->count &&
			!Cmp::greaterThan(KeyOfValue::generate(this, this->data[lowBound]), item);
	}
	int add(const Value& item) {
	    int pos;
  	    find(KeyOfValue::generate(this, item), pos);
		insert(pos, item);
		return pos;
	}
};


// Nice shorthand for arrays with static part
template <typename T, int InlineCapacity>
class HalfStaticArray : public Array<T, InlineStorage<T, InlineCapacity> > {
public:
	explicit HalfStaticArray(MemoryPool& p) : Array<T,InlineStorage<T, InlineCapacity> > (p) {}
	HalfStaticArray(MemoryPool& p, int InitialCapacity) : 
		Array<T, InlineStorage<T, InlineCapacity> > (p, InitialCapacity) {}
	explicit HalfStaticArray(MemoryPool* p) : Array<T,InlineStorage<T, InlineCapacity> > (p) {}
	HalfStaticArray(MemoryPool* p, int InitialCapacity) : 
		Array<T, InlineStorage<T, InlineCapacity> > (p, InitialCapacity) {}
	explicit HalfStaticArray(int InitialCapacity) : 
		Array<T, InlineStorage<T, InlineCapacity> > (InitialCapacity) {}
	HalfStaticArray() : Array<T, InlineStorage<T, InlineCapacity> > () {}
};

}	// Firebird

#endif

