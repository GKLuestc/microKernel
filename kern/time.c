#include <kern/time.h>
#include <inc/assert.h>

// 10毫秒加一次，无符号整型上限 2^32-1 = 4294967295, 相当于可用运行1.3年
static unsigned int ticks;

void
time_init(void)
{
	ticks = 0;
}

// This should be called once per timer interrupt.  A timer interrupt
// fires every 10 ms.
void
time_tick(void)
{
	ticks++;
	if (ticks * 10 < ticks)
		panic("time_tick: time overflowed");
}

unsigned int
time_msec(void)
{
	return ticks * 10;
}
