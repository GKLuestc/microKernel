/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

	TRAPHANDLER_NOEC(th0, 0)
	TRAPHANDLER_NOEC(th1, 1)
	TRAPHANDLER_NOEC(th3, 3)
	TRAPHANDLER_NOEC(th4, 4)
	TRAPHANDLER_NOEC(th5, 5)
	TRAPHANDLER_NOEC(th6, 6)
	TRAPHANDLER_NOEC(th7, 7)
	TRAPHANDLER(th8, 8)
	TRAPHANDLER_NOEC(th9, 9)
	TRAPHANDLER(th10, 10)
	TRAPHANDLER(th11, 11)
	TRAPHANDLER(th12, 12)
	TRAPHANDLER(th13, 13)	// 一般保护故障（当用户权限主动发起缺页中断，会被转到13号一般中断上）
	TRAPHANDLER(th14, 14)	// 缺页中断，只能由内核权限发起
	TRAPHANDLER_NOEC(th16, 16)

	TRAPHANDLER_NOEC(th_syscall, T_SYSCALL)

/*
 * Lab 3: Your code here for _alltraps
 */

	//参考inc/trap.h中的Trapframe结构。tf_ss，tf_esp，tf_eflags，tf_cs，tf_eip，tf_err在中断发生时由处理器压入，所以现在只需要压入剩下寄存器（%ds,%es,通用寄存器）
	//切换到内核数据段
	_alltraps:
    pushl %ds         // 保存段寄存器 DS
    pushl %es         // 保存段寄存器 ES
    pushal            // 保存所有通用寄存器
    pushl $GD_KD      // 压入内核数据段选择子
    popl %ds          // 加载内核数据段到 DS
    pushl $GD_KD      // 再次压入内核数据段选择子
    popl %es          // 加载内核数据段到 ES
    pushl %esp        // 压入当前栈指针（指向 Trapframe 结构的起始地址）
    call trap         // 调用 C 函数 trap()

	//调用call trap之后，不会从这里ret返回，而是在trap函数中，直接根据发起中断的用户环境设置cpu，实现返回