#ifndef __INTERLOCK_H
#define __INTERLOCK_H


#ifdef SOLARIS
extern "C"
{
extern int cas (volatile int *state, int compare, int exchange);
extern void* casx (volatile void **state, void* compare, void* exchange);
}
#define COMPARE_EXCHANGE(target,compare,exchange)\
	(cas(target,compare,exchange)== compare)
#define COMPARE_EXCHANGE_POINTER(target,compare,exchange)\
	(casx((volatile void**)target,(void*)compare,(void*)exchange)== compare)
#endif


#ifdef _WIN32

#define INTERLOCKED_INCREMENT(variable)	InterlockedIncrement (&variable)
#define INTERLOCKED_DECREMENT(variable)	InterlockedDecrement (&variable)

//#define INTERLOCK_TYPE	LONG
#define COMPARE_EXCHANGE(target,compare,exchange)\
	(InterlockedCompareExchange(target,exchange,compare)==compare)
#define COMPARE_EXCHANGE_POINTER(target,compare,exchange)\
	(InterlockedCompareExchangePointer((volatile PVOID*) target,(PVOID)exchange,(PVOID)compare)==compare)
#ifndef InterlockedIncrement
#define InterlockedIncrement		_InterlockedIncrement
#define InterlockedDecrement		_InterlockedDecrement
#define InterlockedCompareExchange	_InterlockedCompareExchange

extern "C" 
	{
	INTERLOCK_TYPE InterlockedIncrement(volatile INTERLOCK_TYPE* lpAddend);
	INTERLOCK_TYPE InterlockedDecrement(volatile INTERLOCK_TYPE* lpAddend);
	INTERLOCK_TYPE InterlockedCompareExchange(volatile INTERLOCK_TYPE *target, INTERLOCK_TYPE exchange, INTERLOCK_TYPE compare);
	}
	
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedCompareExchange)
#endif

#endif

#ifndef INTERLOCKED_INCREMENT
#define INTERLOCKED_INCREMENT(variable)	interlockedIncrement (&variable)
#define INTERLOCKED_DECREMENT(variable)	interlockedDecrement (&variable)
#endif

#if defined (__i386) || (__x86_64__)
#define COMPARE_EXCHANGE(target,compare,exchange)\
	(inline_cas(target,compare,exchange)== compare)
#define COMPARE_EXCHANGE_POINTER(target,compare,exchange)\
	(inline_cas_pointer((volatile void**)target,(void*)compare,(void*)exchange)== compare)
#endif

#ifndef INTERLOCK_TYPE
#define INTERLOCK_TYPE	int
#endif

inline INTERLOCK_TYPE interlockedIncrement(volatile INTERLOCK_TYPE *ptr)
{
#ifdef _WIN32
	return InterlockedIncrement ((INTERLOCK_TYPE*) ptr);
#elif defined (__i386) || (__x86_64__)
	INTERLOCK_TYPE ret;
	asm (
		"mov $1,%%ebx\n\t"
		"lock\n\t" "xaddl %%ebx,%1\n\t" 
		"incl %%ebx\n\t"
		"movl %%ebx,%0\n\t"
		: "=m" (ret)
		: "m" (*ptr) 
		: "%ebx"
		);
	return ret;
#else
	for (;;)
		{
		int oldValue = *ptr;
		int newValue = oldValue + 1;
		if (COMPARE_EXCHANGE(ptr,oldValue,newValue))
			return newValue;
		}
#endif 
}

inline INTERLOCK_TYPE interlockedDecrement(volatile INTERLOCK_TYPE *ptr)
{
#ifdef _WIN32
	return InterlockedDecrement ((INTERLOCK_TYPE*) ptr);
#elif defined (__i386) || (__x86_64__)
	INTERLOCK_TYPE ret;
	asm (
		"mov $-1,%%ebx\n\t"
		"lock\n\t" "xaddl %%ebx,%1\n\t" 
		"decl %%ebx\n\t"
		"movl %%ebx,%0\n\t"
		: "=m" (ret)
		: "m" (*ptr) 
		: "%ebx"
		);
	return ret;
#else
	for (;;)
		{
		int oldValue = *ptr;
		int newValue = oldValue - 1;
		if (COMPARE_EXCHANGE(ptr,oldValue,newValue))
			return newValue;
		}
#endif 
}

inline int inline_cas (volatile int *target, int compare, int exchange)
{
#if defined(__i386) || (__x86_64__)
	int result;
 
	__asm __volatile ("lock; cmpxchgl %2, %1"
		    : "=a" (result), "=m" (*target)
		    : "r" (exchange), "m" (*target), "0" (compare)); 

	return result;

#else
	return -2;
#endif
}

inline void* inline_cas_pointer (volatile void **target, void *compare, void *exchange)
{
#if defined(__i386)
	void* result;
 
	__asm __volatile ("lock; cmpxchgl %2, %1"
		    : "=a" (result), "=m" (*target)
		    : "r" (exchange), "m" (*target), "0" (compare)); 

	return result;

#elif defined(__x86_64__)
	void* result;
 
	__asm __volatile ("lock; cmpxchgq %q2, %1"
		    : "=a" (result), "=m" (*target)
		    : "r" ((long)(exchange)), "m" (*target), "0" ((long)(compare))); 

	return result;

/***
 #define __arch_compare_and_exchange_val_64_acq(mem, newval, oldval) \
   ({ __typeof (*mem) ret;						      \
-     __asm __volatile (LOCK "cmpxchgq %q2, %1"				      \
+     __asm __volatile (LOCK_PREFIX "cmpxchgq %q2, %1"			      \
 		       : "=a" (ret), "=m" (*mem)			      \
 		       : "r" ((long) (newval)), "m" (*mem),		      \
 			 "0" ((long) (oldval)));			      \
***/

#else
	return NULL;
#endif
}

#endif //  __INTERLOCK_H
