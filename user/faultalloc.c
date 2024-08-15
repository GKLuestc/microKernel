// test user-level fault handler -- alloc pages to fix faults

#include <inc/lib.h>

void
handler(struct UTrapframe *utf)
{
	int r;
	void *addr = (void*)utf->utf_fault_va;

	cprintf("fault %x\n", addr);
	if ((r = sys_page_alloc(0, ROUNDDOWN(addr, PGSIZE), PTE_P|PTE_U|PTE_W)) < 0)
		panic("allocating at %x in page fault handler: %e", addr, r);
	
	// 将一个格式化的字符串写入addr指向的内存区域中
	snprintf((char*) addr, 100, "this string was faulted in at %x", addr);
}

void
umain(int argc, char **argv)
{
	set_pgfault_handler(handler);
	cprintf("%s\n", (char*)0xDeadBeef);

	// handler函数会向引起中断的地址中写入 （“this string was faulted in at %x", addr） 这个内容，
	// 所以0xCafeBffe在写入的时候，会进入下一个页面，又需要重新申请页面
	cprintf("%s\n", (char*)0xCafeBffe);    
	// 打印从 0xCafeBffe 开始的 100 个字符
    // cprintf("Content from 0xCafeBffe: %.*s\n", 100, (char*)0xCafeBffe);

    // // 打印从 0xCafec000 开始的 100 个字符
    // cprintf("Content from 0xCafec000: %.*s\n", 100, (char*)0xCafec000);
}
