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
#include <kern/sched.h>
#include <kern/picirq.h>
#include <kern/cpu.h>
#include <kern/spinlock.h>

static void boot_aps(void);


void
i386_init(void)
{
	// Initialize the console.
	// Can't call cprintf until after we do this!
	// 控制台初始化
	cons_init();
	cprintf("6828 decimal is %o octal!\n", 6828);



	// Lab 2 memory management initialization functions
	// 根据mmu.h初始化内存结构，建立映射关系
	// 设置cr0 和 cr3 启动分页机制，相当于启动 CPU 的 MMU 映射
	mem_init();


	// Lab 3 user environment initialization functions
	// 初始化环境链表，初步初始化GDT全局描述符，设置段的权限，预留TSS段
	env_init();


	// trap_init 函数的作用是初始化陷阱（trap）处理机制，以便操作系统能够正确处理各种陷阱和中断。
	// 初始化中断 IDT表，
	// 设置cpu的 TSS 段和 IDT 表
	trap_init();				


	// Lab 4 multiprocessor initialization functions
	// 读取多核配置，写入cpu数组
	mp_init();	

	
	// lapic (Local Advanced Programmable Interrupt Controller)
	// 初始化本地APIC，使其能够正常工作。
	lapic_init();


	// Lab 4 multitasking initialization functions  
	// 初始化 8259A 中断控制器.
	pic_init();


	// Acquire the big kernel lock before waking up APs
	// Your code here:
	lock_kernel();	//在启动其他AP核心之前，BSP获取大内核锁


	// Starting non-boot CPUs
	// 启动 AP
	boot_aps();	

	cprintf("***************** System Init Over!! *****************\n\n");
	
	// Start fs.
	ENV_CREATE(fs_fs, ENV_TYPE_FS);

#if defined(TEST)
	// Don't touch -- used by grading script!
	ENV_CREATE(TEST, ENV_TYPE_USER);
#else
	// Touch all you want.
	// ENV_CREATE(user_pingpong, ENV_TYPE_USER);
	ENV_CREATE(user_icode, ENV_TYPE_USER);
#endif // TEST*

	// Should not be necessary - drains keyboard because interrupt has given up.
	kbd_intr();

	// Schedule and run the first user environment!
	// 调度器。调度之后，选择一个环境运行
	sched_yield();
}

// While boot_aps is booting a given CPU, it communicates the per-core
// stack pointer that should be loaded by mpentry.S to that CPU in
// this variable.
void *mpentry_kstack;

// Start the non-boot (AP) processors.
static void
boot_aps(void)
{
	// 在mpentry.S文件中定义了mpentry_start 和 mpentry_end，这两个位置之间是一段汇编代码
	// 用于初始化保护模式，根据mpentry_kstack变量为cpu设置堆栈，然后跳转到 mp_main 函数
	extern unsigned char mpentry_start[], mpentry_end[];
	void *code;
	struct CpuInfo *c;

	// Write entry code to unused memory at MPENTRY_PADDR
	// 将mpentry.S的汇编代码，搬到 0x7000(MPENTRY_PADDR) ，这个较低的没有使用的内存中，完成最初的保护模式和分页启动
	code = KADDR(MPENTRY_PADDR);
	memmove(code, mpentry_start, mpentry_end - mpentry_start);

	// Boot each AP one at a time
	for (c = cpus; c < cpus + ncpu; c++) {
		if (c == cpus + cpunum())  // We've started already.
			continue;

		// Tell mpentry.S what stack to use 
		// 计算当前核心的工作堆栈，记得加上保护区域
		// 这个变量会被 mpentry.S使用用于设置AP核堆栈
		mpentry_kstack = percpu_kstacks[c - cpus] + KSTKSIZE;

		// Start the CPU at mpentry_start
		// 让核心从code区域开始启动
		lapic_startap(c->cpu_id, PADDR(code));

		// Wait for the CPU to finish some basic setup in mp_main()
		// 
		while(c->cpu_status != CPU_STARTED)
			;
	}
}

// Setup code for APs
// AP 核会启动的代码
void
mp_main(void)
{
	// We are in high EIP now, safe to switch to kern_pgdir 
	lcr3(PADDR(kern_pgdir));
	cprintf("SMP: CPU %d starting\n", cpunum());

	lapic_init();	//初始化本地apic
	env_init_percpu();	//加载GDT和段描述符
	trap_init_percpu();	//初始化和加载 CPU的 TSS和 IDT

	// 将当前cpu的状态设置为运行状态，告诉 bsp，这个 ap 已经启动完成
	xchg(&thiscpu->cpu_status, CPU_STARTED); // tell boot_aps() we're up

	// Now that we have finished some basic setup, call sched_yield()
	// to start running processes on this CPU.  But make sure that
	// only one CPU can enter the scheduler at a time!
	//
	// Your code here:
	lock_kernel();	//	自旋锁，获取内核锁
	sched_yield();	// 	启动调度器
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
	cprintf("kernel panic on CPU %d at %s:%d: ", cpunum(), file, line);
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
