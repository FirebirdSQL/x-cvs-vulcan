#ifndef _OLD_VECTOR_H_
#define _OLD_VECTOR_H_

#ifdef MEMMGR
#include "MemoryManager.h"
/***
class MemMgr;
#ifndef MemoryPool
#define MemoryPool	::MemMgr
#endif
***/
#endif

#define FOR_VEC(type,name,vector) {int index = 0;\
		for (vec::iterator iter = vector->begin(), end = vector->end(); iter < end; iter++, index++)\
			{ type name = (type) *iter;

#ifndef END_FOR
#define END_FOR	}}
#endif

/* general purpose vector */
template <class T, USHORT TYPE = type_vec>
class vec_base //: protected pool_alloc<TYPE>
{
public:
	typedef typename firebird::vector<T>::iterator iterator;
	typedef typename firebird::vector<T>::const_iterator const_iterator;

	static vec_base* newVector(MemoryPool& p, int len)
		{ return FB_NEW(p) vec_base<T,TYPE>(p, len); }
		
	static vec_base* newVector(MemoryPool& p, const vec_base& base)
		{ 
		return FB_NEW(p) vec_base<T,TYPE>(p, base); 
		}
		
	// CVC: THis should be size_t instead of ULONG for maximum portability.
	ULONG count() const { return (ULONG) vector.size(); }
	T& operator[](size_t index) { return vector[index]; }
	const T& operator[](size_t index) const { return vector[index]; }

	iterator begin() { return vector.begin(); }
	iterator end() { return vector.end(); }
	
	void clear() { vector.clear(); }
	void prepend(int n) { vector.insert(vector.begin(), n); }
	
//	T* memPtr() { return &*(vector.begin()); }
	T* memPtr() { return &vector[0]; }

	void resize(size_t n, T val = T()) { vector.resize(n, val); }

	void operator delete(void* mem) { MemoryPool::globalFree(mem); }

protected:
	vec_base(MemoryPool& p, int len) : vector(len, p, TYPE) 
		{
		}
		
	vec_base(MemoryPool& p, const vec_base& base) : vector(p, TYPE) 
		{ 
		vector = base.vector; 
		}

private:
	firebird::vector<T> vector;
};

class vec : public vec_base<blk*, type_vec>
{
public:
    static vec* newVector(MemoryPool& p, int len)
        { return FB_NEW(p) vec(p, len); }
    static vec* newVector(MemoryPool& p, const vec& base)
        { return FB_NEW(p) vec(p, base); }
	static vec* newVector(MemoryPool& p, vec* base, int len)
		{
			if (!base)
				base = FB_NEW(p) vec(p, len);
			else if (len > (int) base->count())
				base->resize(len);
			return base;
		}

private:
    vec(MemoryPool& p, int len) : vec_base<blk*, type_vec>(p, len) {}
    vec(MemoryPool& p, const vec& base) : vec_base<blk*, type_vec>(p, base) {}
};
typedef vec* VEC;

class vcl : public vec_base<SLONG, type_vcl>
{
public:
    static vcl* newVector(MemoryPool& p, int len)
        { return FB_NEW(p) vcl(p, len); }
    static vcl* newVector(MemoryPool& p, const vcl& base)
        { return FB_NEW(p) vcl(p, base); }
	static vcl* newVector(MemoryPool& p, vcl* base, int len)
		{
			if (!base)
				base = FB_NEW(p) vcl(p, len);
			else if (len > (int) base->count())
				base->resize(len);
			return base;
		}

private:
    vcl(MemoryPool& p, int len) : vec_base<SLONG, type_vcl>(p, len) {}
    vcl(MemoryPool& p, const vcl& base) : vec_base<SLONG, type_vcl>(p, base) {}
};
typedef vcl* VCL;

#define TEST_VECTOR(vector,number)      ((vector && number < vector->vec_count) ? \
					  vector->vec_object [number] : NULL)


#endif
