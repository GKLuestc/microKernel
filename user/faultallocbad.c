// test user-level fault handler -- alloc pages to fix faults
// doesn't work because we sys_cputs instead of cprintf (exercise: why?)

#include <inc/lib.h>

void
handler(struct UTrapframe *utf)
{
	int r;
	void *addr = (void*)utf->utf_fault_va;

	cprintf("fault %x\n", addr);
	if ((r = sys_page_alloc(0, ROUNDDOWN(addr, PGSIZE),	PTE_P|PTE_U|PTE_W)) < 0)
		panic("allocating at %x in page fault handler: %e", addr, r);
	snprintf((char*) addr, 100, "this string was faulted in at %x\n", addr);
}

void
umain(int argc, char **argv)
{
	set_pgfault_handler(handler);

	// 直接调用 sys_cputs系统调用，检查发现这个页面不存在或者权限不足，会直接销毁当前环境
	// 所以在使用 sys_cputs系统调用之前，必须保证页面存在且权限正确
	// 在用户程序对不存在的页面进行任何操作，都会触发用户缺页异常，进入注册的handler处理函数
	// 所以不要在用户程序中，直接单独使用 sys_cputs 调用

	// *(char*)0xDeadBeef = 'G';
	sys_cputs((char*)0xDEADBEEF, 100);
}
