#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include <sched.h>

typedef volatile char spinlock_t;

#define SPINLOCK_INITIALIZER 0

static __inline __attribute__((always_inline, no_instrument_function))
char xchg_8(volatile void *ptr, char x)
{
	__asm__ __volatile__("xchgb %0, %1"
			:"=r" (x)
			:"m" (*(volatile char *)ptr), "0" (x)
			:"memory");

	return x;
}

#define spinlock_lock(lock) \
	do { \
		while (1) { \
			if (!xchg_8(lock, 1)) { \
				break; \
			} \
			while (*lock) { \
				__asm__ __volatile__("pause\n": : :"memory"); \
			} \
		} \
	} while (0)

#define spinlock_unlock(lock) \
	do { \
		*lock = 0; \
	} while (0)

#define spinlock_trylock(lock) \
	do { \
		xchg_8(lock, 1); \
	} while (0)

#define cspinlock_lock(lock) \
	do { \
		__asm__ __volatile__( \
				"jmp 2f\n\t" \
				"1: pause\n\t" \
				"2: xorl %%eax, %%eax\n\t" \
				"lock cmpxchgb %b1, %0\n\t" \
				"jne 1b\n" \
				:"+m" (*(volatile char *)lock) \
				:"r" (1) \
				:"memory", "cc", "eax"); \
	} while (0)

#define cspinlock_unlock(lock) \
	do { \
		*lock = 0; \
	} while (0)

#define gspinlock_lock(lock) \
	do { \
		while (!__sync_bool_compare_and_swap(lock, 0, 1)) { \
			sched_yield(); \
		} \
	} while(0)

#define gspinlock_unlock(lock) \
	do { \
		*lock = 0; \
	} while(0)

#endif /* __SPINLOCK_H__ */
