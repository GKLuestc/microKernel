// System call stubs.

#include <inc/syscall.h>
#include <inc/lib.h>

static inline int32_t
syscall(int num, int check, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret;

	// Generic system call: pass system call number in AX,
	// up to five parameters in DX, CX, BX, DI, SI.
	// Interrupt kernel with T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because we don't use the
	// return value.
	//
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.

	// 这里的 1% 相当于取后面的第一个操作数，“i”(T_SYSCALL)
	asm volatile("int %1\n"
		     : "=a" (ret)			// 输出: ret 存储在 EAX 寄存器中
		     : "i" (T_SYSCALL),		// 输入: 中断号存储在立即数 T_SYSCALL 中
		       "a" (num),			// 输入: 系统调用号存储在 EAX 寄存器中
		       "d" (a1),			// 输入: 第一个参数存储在 EDX 寄存器中
		       "c" (a2),			// 输入: 第二个参数存储在 ECX 寄存器中
		       "b" (a3),			// 输入: 第三个参数存储在 EBX 寄存器中
		       "D" (a4),			// 输入: 第四个参数存储在 EDI 寄存器中
		       "S" (a5)				// 输入: 第五个参数存储在 ESI 寄存器中
		     : "cc", "memory");		// 告诉编译器此指令会修改条件码和内存

	if(check && ret > 0)
		panic("syscall %d returned %d (> 0)", num, ret);

	return ret;
}

void
sys_cputs(const char *s, size_t len)
{
	syscall(SYS_cputs, 0, (uint32_t)s, len, 0, 0, 0);
}

int
sys_cgetc(void)
{
	return syscall(SYS_cgetc, 0, 0, 0, 0, 0, 0);
}

int
sys_env_destroy(envid_t envid)
{
	return syscall(SYS_env_destroy, 1, envid, 0, 0, 0, 0);
}

envid_t
sys_getenvid(void)
{
	 return syscall(SYS_getenvid, 0, 0, 0, 0, 0, 0);
}

