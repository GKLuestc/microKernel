// Ping-pong a counter between two processes.
// Only need to start one of these -- splits into two, crudely.

#include <inc/string.h>
#include <inc/lib.h>

envid_t dumbfork(void);

void
umain(int argc, char **argv)
{
	envid_t who;
	int i;

	// fork a child process
	who = dumbfork();

	// print a message and yield to the other a few times
	for (i = 0; i < (who ? 10 : 20); i++) {
		cprintf("%d: I am the %s!\n", i, who ? "parent" : "child");
		sys_yield();
	}
}


// 将父进程addr地址数据，拷贝到子进程addr地址中
// 通过 UTEMP 当做桥来实现
void
duppage(envid_t son_id, void *addr)
{
	int r;
	// 目标，将父进程中addr的页面，复制到子进程addr的页面，且两个页面独立
	// 1、为子进程addr虚拟地址申请一个页面
	// 2、将父进程的UTEMP虚拟地址和子进程的addr虚拟地址，指向同一块物理页面
	// 3、父进程，将addr虚拟地址的数据，拷贝到UTEMP虚拟地址的页面中（由于这个函数由父进程执行，此时memmove中所有的虚拟地址，都通过父进程的页目录进行映射）
	// 4、断开父进程UTEMP虚拟地址的一切映射
	// This is NOT what you should do in your fork.
	if ((r = sys_page_alloc(son_id, addr, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);
	if ((r = sys_page_map(son_id, addr, thisenv->env_id, UTEMP, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_map: %e", r);
	memmove(UTEMP, addr, PGSIZE);
	if ((r = sys_page_unmap(thisenv->env_id, UTEMP)) < 0)
		panic("sys_page_unmap: %e", r);
}


// 模仿Linux系统的fork，生成一个当前进程的复制，使用是直接拷贝父进程内存到子进程中
/*
*为了提高效率，现代操作系统通常采用 写时复制（COW） 技术。
*这个技术的核心思想是：在子进程创建时，不直接复制父进程的内存页，而是让父子进程共享同一块物理页面，
*直到其中一个进程尝试写入这块内存。当发生写入操作时，操作系统会触发页错误异常，然后实际执行页面的复制操作，
*这样两个进程就有了各自独立的页面。这大大减少了不必要的内存复制，提高了内存利用率。
*/ 
envid_t
dumbfork(void)
{
	envid_t envid;
	uint8_t *addr;
	int r;
	
	//						   .text	.rodata	  .data	 	  .bss
	// 定义在user.ld文件中，标志代码段，只读数据段，全局变量段，未初始化全局变量段的结尾
	extern unsigned char end[];

	// Allocate a new child environment.
	// The kernel will initialize it with a copy of our register state,
	// so that the child will appear to have called sys_exofork() too -
	// except that in the child, this "fake" call to sys_exofork()
	// will return 0 instead of the envid of the child.

	// 使用 sys_exofork 系统调用， 会在此处复制父进程的上下文，子进程启动后从此处开始执行
	// 父进程的系统返回值是子进程 ID
	// 子进程的返回值是 0
	envid = sys_exofork();
	// 但是此时的子进程，还是不可运行状态，因为子进程只有父进程的寄存器状态。
	// 缺少代码段，数据段，堆栈段等运行必要的环境

	if (envid < 0)
		panic("sys_exofork: %e", envid);


	// 子进程会进这个 if， 因为子进程的返回值被系统调用 sys_exofork 设置为了0
	if (envid == 0) {
		// We're the child.
		// The copied value of the global variable 'thisenv'
		// is no longer valid (it refers to the parent!).
		// Fix it and return 0.
		// 子进程重新获取自己的 ID
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}


	// 父进程会继续执行这个
	// We're the parent.
	// Eagerly copy our entire address space into the child.
	// This is NOT what you should do in your fork implementation.
	for (addr = (uint8_t*) UTEXT; addr < end; addr += PGSIZE)
	{
		duppage(envid, addr);		
	}

	// 因为这里的 addr,是当前函数中的局部变量，存储在堆栈中，在堆栈中向下页面对齐，肯定是栈的最大顶。因为用户栈只有一个页面大小。
	// 理论上，这里使用  ROUNDDOWN(&任意局部变量, PGSIZE)，都能实现复制堆栈页面, 下面使用print进行证明，是正确的
	// Also copy the stack we are currently running on.
	duppage(envid, ROUNDDOWN(&addr, PGSIZE));

	// 控制台打印结果  ROUNDDOWN(&addr, PGSIZE) = 0xeebfd000, should equial 0xeebfd000  
	// 				 ROUNDDOWN(&envid, PGSIZE) = 0xeebfd000
	cprintf("ROUNDDOWN(&addr, PGSIZE) = 0x%x, should equial 0x%x \n", ROUNDDOWN(&addr, PGSIZE), USTACKTOP-4096);
	cprintf("ROUNDDOWN(&envid, PGSIZE) = 0x%x\n", ROUNDDOWN(&envid, PGSIZE), USTACKTOP-4096);

	// Start the child environment running
	// 将子进程设置为就绪态
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	return envid;
}

