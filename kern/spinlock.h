#ifndef JOS_INC_SPINLOCK_H
#define JOS_INC_SPINLOCK_H

#include <inc/types.h>

// Comment this to disable spinlock debugging
// 自旋锁调试
#define DEBUG_SPINLOCK

// Mutual exclusion lock.
// 互斥锁
struct spinlock {
	unsigned locked;       // Is the lock held?

#ifdef DEBUG_SPINLOCK
	// For debugging:
	char *name;            // 锁的名称		Name of lock.
	struct CpuInfo *cpu;   // 持有锁的CPU	The CPU holding the lock.
	uintptr_t pcs[10];     // 调用堆栈		The call stack (an array of program counters)
	                       // that locked the lock.
#endif
};

void __spin_initlock(struct spinlock *lk, char *name);
void spin_lock(struct spinlock *lk);
void spin_unlock(struct spinlock *lk);

#define spin_initlock(lock)   __spin_initlock(lock, #lock)

extern struct spinlock kernel_lock;


// 上锁，持有锁
static inline void
lock_kernel(void)
{
	spin_lock(&kernel_lock);
}

// 解锁，解除持有状态
static inline void
unlock_kernel(void)
{
	spin_unlock(&kernel_lock);

	// Normally we wouldn't need to do this, but QEMU only runs
	// one CPU at a time and has a long time-slice.  Without the
	// pause, this CPU is likely to reacquire the lock before
	// another CPU has even been given a chance to acquire it.
	asm volatile("pause");
}

#endif
