// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if (!((err & FEC_WR) && (uvpt[PGNUM(addr)] & PTE_COW))) { //只有因为写操作写时拷贝的地址这中情况，才可以抢救。否则一律panic
		panic("pgfault():not cow");
	}


	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	addr = ROUNDDOWN(addr, PGSIZE);
	if ((r = sys_page_map(0, addr, 0, PFTEMP, PTE_U|PTE_P)) < 0) //将当前进程PFTEMP也映射到当前进程addr指向的物理页
		panic("sys_page_map: %e", r);
	if ((r = sys_page_alloc(0, addr, PTE_P|PTE_U|PTE_W)) < 0)	//令当前进程addr指向新分配的物理页
		panic("sys_page_alloc: %e", r);
	memmove(addr, PFTEMP, PGSIZE);								//将PFTEMP指向的物理页拷贝到addr指向的物理页
	if ((r = sys_page_unmap(0, PFTEMP)) < 0)					//解除当前进程PFTEMP映射
		panic("sys_page_unmap: %e", r);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	void *addr = (void*) (pn * PGSIZE);

	if (uvpt[pn] & PTE_SHARE) //对于表示为PTE_SHARE的页，拷贝映射关系，并且两个进程都有读写权限
	{	
		sys_page_map(0, addr, envid, addr, PTE_SYSCALL); 
	} 
	else if ((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW)) //对于UTOP以下的可写的或者写时拷贝的页，拷贝映射关系的同时，需要同时标记当前进程和子进程的页表项为PTE_COW
	{ 
		if ((r = sys_page_map(0, addr, envid, addr, PTE_COW|PTE_U|PTE_P)) < 0)
			panic("sys_page_map:%e", r);
		if ((r = sys_page_map(0, addr, 0, addr, PTE_COW|PTE_U|PTE_P)) < 0)
			panic("sys_page_map:%e", r);
	} else 
	{
		sys_page_map(0, addr, envid, addr, PTE_U|PTE_P);	//对于只读的页，只需要拷贝映射关系即可
	}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// 注册用户缺页异常处理函数
	extern void _pgfault_upcall(void);
	set_pgfault_handler(pgfault);
	envid_t envid;

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
		thisenv = &envs[ENVX(sys_getenvid())];// 子进程重新获取自己的 ID
		return 0;
	}

	// 父进程会继续执行这个（根据写时复制的原则，将父进程映射子进程的页面都设置为只读）
	uint32_t addr;
	for (addr = 0; addr < USTACKTOP; addr += PGSIZE)
	{
		if (   (uvpd[  PDX(addr)] & PTE_P) 	 // uvpd[  PDX(addr)] ，在页目录中是否存在
			&& (uvpt[PGNUM(addr)] & PTE_P) 	 // uvpt[pagenumber]， 能访问到第pagenumber项页表条目是否存在且可读
			&& (uvpt[PGNUM(addr)] & PTE_U))  // 
		{
			duppage(envid, PGNUM(addr));	//拷贝当前进程映射关系到子进程
		}
	}

	int r;
	// 映射父进程的栈区
	if ((r = sys_page_alloc(envid, (void *)(UXSTACKTOP-PGSIZE), PTE_P | PTE_W | PTE_U)) < 0)	//为子进程分配异常栈
		panic("sys_page_alloc: %e", r);

	//为子进程设置 _pgfault_upcall，注册异常处理
	sys_env_set_pgfault_upcall(envid, _pgfault_upcall);		

	// 父进程设置完成，将子进程设置为就绪态
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);	

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
