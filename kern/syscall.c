/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.
	user_mem_assert(curenv, s, len, 0);

	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	// 如果查不到环境 id，直接返回错误
	if ((r = envid2env(envid, &e, 1)) < 0){
		panic("sys_env_destroy->envid2env: %e", r);
		return r;		
	}

	// 打印销毁信息，是自身销毁，还是当前环境销毁其他环境

	// 销毁环境
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.
	// LAB 4: Your code here.
	struct Env *e;

	int ret = env_alloc(&e, curenv->env_id);

	if( ret < 0 )
		return ret;

	// 将父环境的寄存器值赋给子环境, 相当于继承父类的工作状态。
	// 保证子进程，在启动的时候，也会从中断位置返回，从而接收中断返回值0
	e->env_tf = curenv->env_tf;
	
	// 设置子环境状态为不可运行
	e->env_status = ENV_NOT_RUNNABLE;
	
	// 将子环境的 eax 返回值置为 0 
	e->env_tf.tf_regs.reg_eax = 0;

	// 父环境的返回值,是子类的 ID，通过return来获取
	return e->env_id;

}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.	
	// LAB 4: Your code here.
	struct Env *e;
	int ret = envid2env(envid, &e, 1);

	if( ret < 0 ){
		return ret;
	}

	e->env_status = status;
	return 0;
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3), interrupts enabled, and IOPL of 0.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	// LAB 5: Your code here.
	// Remember to check whether the user has supplied us with a good
	// address!
	panic("sys_env_set_trapframe not implemented");
}


// 缺页中断发生时，会执行env_pgfault_upcall指定位置的代码。
// 当执行env_pgfault_upcall指定位置的代码时，栈已经转到异常栈，并且压入了UTrapframe结构
// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	struct Env *e;
	int ret = envid2env(envid, &e, 1);
	if(ret < 0) return ret;

	e->env_pgfault_upcall = func;
	return 0;
	
	// LAB 4: Your code here.
	// panic("sys_env_set_pgfault_upcall not implemented");
}

// 为环境 envid的 va虚拟地址，申请一块页面
// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.
	struct Env *e;
	int ret = envid2env(envid, &e, 1);
	if( ret  ) {
		cprintf("envid = %d\n",envid);
		return ret;
	}

	// 如果虚拟地址超出用户区，或者不对齐页表，返回 -E_INVAL
	if( va >= (void*)UTOP || (ROUNDDOWN(va, PGSIZE) != va) ){
		return -E_INVAL;
	}

	// 如果权限不满足，返回 -E_INVAL
	int flag = PTE_U | PTE_P;
	if((perm & flag) != flag){
		return -E_INVAL;
	}

	// 申请页面
	struct PageInfo *pp = page_alloc(1);
	if(pp == NULL){
		return -E_NO_MEM;
	} 
	pp->pp_ref++;

	// 将虚拟地址和页面对应
	ret = page_insert(e->env_pgdir, pp, va, perm);
	if( ret < 0 ){
		page_free(pp);
		return ret;
	}

	return 0;
}


// 让 dstva 和 srcva 地址映射同一块页面
// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva, envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.
	// 判断两个环境是否正常存在
	struct Env *se, *de;

	int ret = envid2env(srcenvid, &se, 1);
	if (ret){
		return ret;	//bad_env
	} 

	ret = envid2env(dstenvid, &de, 1);
	if (ret){
		return ret;	//bad_env
	} 

	// cprintf("se->env_pgdir = 0x%x\n",(se->env_pgdir));
	// cprintf("de->env_pgdir = 0x%x\n",(de->env_pgdir));


	// 对源地址和目标地址进行判断
	if (srcva >= (void*)UTOP || dstva >= (void*)UTOP || 
		ROUNDDOWN(srcva,PGSIZE) != srcva || ROUNDDOWN(dstva,PGSIZE) != dstva) 
	{
		return -E_INVAL;			
	}


	// 查询源地址的页面
	pte_t *pte;
	struct PageInfo *pg = page_lookup(se->env_pgdir, srcva, &pte);
	if (!pg) {
		return -E_INVAL;
	}

	// 权限判断
	int flag = PTE_U|PTE_P;
	if ((perm & flag) != flag){
		return -E_INVAL;
	} 

	if (((*pte & PTE_W) == 0) && (perm & PTE_W)){
		return -E_INVAL;
	} 

	// 进行页面映射
	ret = page_insert(de->env_pgdir, pg, dstva, perm);
	if( ret < 0 ){
		return -E_NO_MEM;
	}

	return 0;
}

// 取消环境 envid 的 va 虚拟地址对应的映射关系
// （虚拟地址必须范围合理，且页面对齐）
// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 4: Your code here.
	struct Env *e;
	int ret = envid2env(envid, &e, 1);
	if( ret < 0 ) return ret;

	// 判断虚拟地址范围合理，以及是否页面对其
	if ((va >= (void*)UTOP) || (ROUNDDOWN(va, PGSIZE) != va)) return -E_INVAL;
	
	// 取消映射
	page_remove(e->env_pgdir, va);
	return 0;
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid'sz
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 4: Your code here.
	struct Env *rcvenv;
	int ret;
	int debug = 0;

	// 如果没有查到环境
	if( (ret = envid2env(envid, &rcvenv, 0)) < 0){
		return ret;
	}

	// 如果目标环境并不需要接收数据
	if(rcvenv->env_ipc_recving == 0){
		return -E_IPC_NOT_RECV;
	}

	// 如果发送页面，属于用户空间，没有超限
	// 并且接收者也需要接收一页数据，在env_ipc_dstva虚拟地址中
	if(srcva < (void*)UTOP && rcvenv->env_ipc_dstva < (void*)UTOP){
		//没有页面对齐
		if(srcva != ROUNDDOWN(srcva, PGSIZE)){
			if (debug) {
				cprintf("sys_ipc_try_send():srcva is not page-alligned\n");
			}
			return -E_INVAL;
		}

		//查找虚拟地址映射的页表项
		pte_t *pte;
		struct PageInfo *pg = page_lookup(curenv->env_pgdir, srcva, &pte);
		if (debug) {
			cprintf("sys_ipc_try_send():srcva=%08x\n", (uintptr_t)srcva);
		}

		//权限检查不合适
		if( (*pte & perm & 7) != (perm & 7) ){	
			if (debug) {
				cprintf("sys_ipc_try_send():perm is wrong\n");
			}
			return -E_INVAL;
		}

		//srcva还没有映射到物理页
		if(pg == NULL){
			if (debug) {
				cprintf("sys_ipc_try_send():srcva is not maped\n");
			}
			return -E_INVAL;
		}

		// 如果没有写权限
		if ((perm & PTE_W) && !(*pte & PTE_W)) {
			if (debug) {
				cprintf("sys_ipc_try_send():*pte do not have PTE_W, but perm have\n");
			}
			return -E_INVAL;
		}		

		// 映射相同页面
		ret = page_insert(rcvenv->env_pgdir, pg, rcvenv->env_ipc_dstva, perm); //共享相同的映射关系
		if (ret < 0) return ret;
		rcvenv->env_ipc_perm = perm;
	}

	// 将接收者标记为不可再次接收，防止传递的数据被覆盖
	rcvenv->env_ipc_recving = 0;

	// 标记此次传输的发生者 ID
	rcvenv->env_ipc_from = curenv->env_id;
	
	// 记录此次传输的单数据
	rcvenv->env_ipc_value = value;
	
	// 将接收者，从阻塞态转为就绪态
	rcvenv->env_status = ENV_RUNNABLE;
	
	// 将接收者的 eax 寄存器设为 0 ，也就是中断返回值
	rcvenv->env_tf.tf_regs.reg_eax = 0;

	return 0;

}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 4: Your code here.
	// 表示愿意接收一页数据，但是没有页面对齐
	if( dstva < (void*)UTOP && dstva != ROUNDDOWN(dstva, PGSIZE)){
		return 	-E_INVAL;
	}

	//设置当前环境等待接收一个数据
	curenv->env_ipc_recving = 1;

	// 设置当前环境为阻塞态
	curenv->env_status = ENV_NOT_RUNNABLE;
	
	// 记录当前环境需要接收页面的映射地址
	curenv->env_ipc_dstva = dstva;

	return 0;
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.

	// panic("syscall not implemented");

	int32_t ret;
	switch (syscallno) {    //根据系统调用号调用相应函数

		case SYS_cputs:		//调用系统输出，a1字符串指针，a2是长度
			sys_cputs((char *)a1, (size_t)a2);
			ret = 0;
			break;

		case SYS_cgetc:		//从控制台读取一个字符
			ret = sys_cgetc();
			break;

		case SYS_getenvid:	// 获取当前用户环境	ID
			ret = sys_getenvid();
			break;

		case SYS_env_destroy:	// 销毁当前环境，a1存储环境 ID
			ret = sys_env_destroy((envid_t)a1);
			break;

		case SYS_yield:		//请求调度中断
			ret = 0;
			sys_yield();
			break;

		case SYS_exofork:		
			/*
				这个系统创建一个新的环境，几乎空白的环境：没有地址空间的映射，不可运行。
				当调用该系统调用时，新环境会和父环境拥有相同的寄存器状态。
				在父环境中，sys_exofork 将返回新创建的环境的 envid_t(如果环境分配失败，则返回负的错误代码)。然而，在子进程中，它将返回 0。
				(由于子进程一开始被标记为不可运行，sys_exofork 实际上不会在子进程中返回，直到父进程使用....显式地将子进程标记为可运行。)
			*/
			ret = sys_exofork();
			break;
		
		case SYS_env_set_status:
			/*
				设置指定环境的状态为 ENV_RUNNABLE 或 ENV_NOT_RUNNABLE。
				这个系统调用通常用于标记一个新环境准备运行，一旦它的地址空间和寄存器状态已经完全初始化。
			*/
			ret = sys_env_set_status((envid_t)a1, (int)a2);
			break;
		

		case SYS_page_alloc:
			/*
				分配一页物理内存，并将其映射到给定环境的地址空间中的给定虚拟地址。
			*/
			ret = sys_page_alloc((envid_t)a1, (void *)a2, (int)a3);
			break;


		case SYS_page_map:
			/*
				将一个页面映射(而不是页面的内容!)从一个环境的地址空间复制到另一个环境，
				保留一个内存共享安排，以便新映射和旧映射都引用物理内存的同一页。
			*/
			ret = sys_page_map((envid_t)a1, (void *)a2, (envid_t)a3, (void *)a4, (int)a5 );
			break;

		case SYS_page_unmap:
			/*
				取消在给定环境中给定虚拟地址映射的页的映射。
			*/
			ret = sys_page_unmap((envid_t)a1, (void *)a2);
			break;
		
		// 注册缺页中断异常处理函数
		case SYS_env_set_pgfault_upcall:
			ret = sys_env_set_pgfault_upcall((envid_t)a1, (void *)a2);
			break;
	
		// IPC 进程间通信，发送中断函数
		case SYS_ipc_try_send:
			ret = sys_ipc_try_send((envid_t) a1, (uint32_t)a2, (void *)a3, (unsigned)a4);
			break;

		// IPC 进程间通信，接收中断函数
		case SYS_ipc_recv:
			ret = sys_ipc_recv((void *)a1);
			break;

		default:
			return -E_INVAL;
	}
	
	return ret;
}

