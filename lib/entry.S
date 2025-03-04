#include <inc/mmu.h>
#include <inc/memlayout.h>

.data
// Define the global symbols 'envs', 'pages', 'uvpt', and 'uvpd'
// so that they can be used in C as if they were ordinary global arrays.
// 在 pmap.c 中，已经将内核的页目录放入 UVPT 这个地址空间中了，方便用户程序进行
// 在虚拟内存分布中，UVPT所在位置，正好是2^22，能放下所有的页表首地址 
// (UVPT>>12)*4, 相当于查找UVPT这个虚拟地址所在的页表首地址，查到正好是页目录位置

	.globl envs
	.set envs, UENVS
	.globl pages
	.set pages, UPAGES
	.globl uvpt
	.set uvpt, UVPT
	.globl uvpd
	.set uvpd, (UVPT+(UVPT>>12)*4)


// 所有运行环境的起始程序，跳转到libmain程序
// Entrypoint - this is where the kernel (or our parent environment)
// starts us running when we are initially loaded into a new environment.
.text
.globl _start
_start:
	// See if we were started with arguments on the stack
	cmpl $USTACKTOP, %esp
	jne args_exist

	// If not, push dummy argc/argv arguments.
	// This happens when we are loaded by the kernel,
	// because the kernel does not know about passing arguments.
	pushl $0
	pushl $0

args_exist:
	call libmain
1:	jmp 1b

