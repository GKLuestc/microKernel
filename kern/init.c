/* See COPYRIGHT for copyright information. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/monitor.h>
#include <kern/console.h>
#include <kern/pmap.h>
#include <kern/kclock.h>
#include <kern/env.h>
#include <kern/trap.h>


void
i386_init(void)
{
	extern char edata[], end[];

	// cprintf("edata addr =  0x%x \n", (uint32_t)((char *)edata));
	// cprintf("end addr =  0x%x \n", (uint32_t)((char *)end));	
	// Before doing anything else, complete the ELF loading process.
	// Clear the uninitialized global data (BSS) section of our program.
	// This ensures that all static/global variables start out zero.
	// 完成ELF后续工作，将 .bss段的全局变量和静态变量清零
	memset(edata, 0, end - edata);

	// Initialize the console.
	// Can't call cprintf until after we do this!
	// 控制台初始化
	cons_init();

	cprintf("6828 decimal is %o octal!\n", 6828);

	// Lab 2 memory management initialization functions
	// 根据mmu.h初始化内存结构，建立映射关系
	// 设置cr0 和 cr3 启动分页机制
	mem_init();

	// Lab 3 user environment initialization functions
	// 初始化环境链表，初步初始化GDT全局描述符，设置段的权限，预留TSS段
	env_init();

	// trap_init 函数的作用是初始化陷阱（trap）处理机制，以便操作系统能够正确处理各种陷阱和中断。
	// 初始化中断 IDT表，
	// 设置cpu的 TSS 段和 IDT 表
	trap_init();	
					
	cprintf("***************** System Init Over!! *****************\n\n");


#if defined(TEST)
	// Don't touch -- used by grading script!
	ENV_CREATE(TEST, ENV_TYPE_USER);
#else
	// Touch all you want.
	// 设置用户链接名称 “_binary_obj_ + user_hello + _start”
	// 申请一个空用户，加载用户程序的ELF程序
	// 设置用户结构体的寄存器数据
	ENV_CREATE(user_hello, ENV_TYPE_USER);
#endif // TEST*

	// We only have one user environment for now, so just run it.
	// 切换到某个用户环境
	// 1、切换页目录；2、切用户环境的寄存器(切换cpu执行权)
	env_run(&envs[0]);
}


/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
const char *panicstr;



/*
 * 当程序错误，打印file，直接启动 monitor 函数
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters the kernel monitor.
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	if (panicstr)
		goto dead;
	panicstr = fmt;

	// Be extra sure that the machine is in as reasonable state
	asm volatile("cli; cld");

	va_start(ap, fmt);
	cprintf("kernel panic at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);

dead:
	/* break into the kernel monitor */
	while (1)
		monitor(NULL);
}

/* like panic, but don't */
void
_warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	cprintf("kernel warning at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);
}
