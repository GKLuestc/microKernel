#ifndef JOS_INC_X86_H
#define JOS_INC_X86_H

#include <inc/types.h>

static inline void
breakpoint(void)
{
	asm volatile("int3");
}

static inline uint8_t
inb(int port)
{
	uint8_t data;
	asm volatile("inb %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static inline void
insb(int port, void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\tinsb"
		     : "=D" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "memory", "cc");
}

static inline uint16_t
inw(int port)
{
	uint16_t data;
	asm volatile("inw %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static inline void
insw(int port, void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\tinsw"
		     : "=D" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "memory", "cc");
}

static inline uint32_t
inl(int port)
{
	uint32_t data;
	asm volatile("inl %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static inline void
insl(int port, void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\tinsl"
		     : "=D" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "memory", "cc");
}

static inline void
outb(int port, uint8_t data)
{
	asm volatile("outb %0,%w1" : : "a" (data), "d" (port));
}

static inline void
outsb(int port, const void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\toutsb"
		     : "=S" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "cc");
}

static inline void
outw(int port, uint16_t data)
{
	asm volatile("outw %0,%w1" : : "a" (data), "d" (port));
}

static inline void
outsw(int port, const void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\toutsw"
		     : "=S" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "cc");
}

static inline void
outsl(int port, const void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\toutsl"
		     : "=S" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "cc");
}

static inline void
outl(int port, uint32_t data)
{
	asm volatile("outl %0,%w1" : : "a" (data), "d" (port));
}

static inline void
invlpg(void *addr)
{
	asm volatile("invlpg (%0)" : : "r" (addr) : "memory");
}

static inline void
lidt(void *p)
{
	asm volatile("lidt (%0)" : : "r" (p));
}

static inline void
lgdt(void *p)
{
	asm volatile("lgdt (%0)" : : "r" (p));
}

static inline void
lldt(uint16_t sel)
{
	asm volatile("lldt %0" : : "r" (sel));
}

static inline void
ltr(uint16_t sel)
{
	asm volatile("ltr %0" : : "r" (sel));
}

static inline void
lcr0(uint32_t val)
{
	asm volatile("movl %0,%%cr0" : : "r" (val));
}

static inline uint32_t
rcr0(void)
{
	uint32_t val;
	asm volatile("movl %%cr0,%0" : "=r" (val));
	return val;
}

static inline uint32_t
rcr2(void)
{
	uint32_t val;
	asm volatile("movl %%cr2,%0" : "=r" (val));
	return val;
}

static inline void
lcr3(uint32_t val)
{
	asm volatile("movl %0,%%cr3" : : "r" (val));
}

static inline uint32_t
rcr3(void)
{
	uint32_t val;
	asm volatile("movl %%cr3,%0" : "=r" (val));
	return val;
}

static inline void
lcr4(uint32_t val)
{
	asm volatile("movl %0,%%cr4" : : "r" (val));
}

static inline uint32_t
rcr4(void)
{
	uint32_t cr4;
	asm volatile("movl %%cr4,%0" : "=r" (cr4));
	return cr4;
}

static inline void
tlbflush(void)
{
	uint32_t cr3;
	asm volatile("movl %%cr3,%0" : "=r" (cr3));
	asm volatile("movl %0,%%cr3" : : "r" (cr3));
}

static inline uint32_t
read_eflags(void)
{
	uint32_t eflags;
	asm volatile("pushfl; popl %0" : "=r" (eflags));
	return eflags;
}

static inline void
write_eflags(uint32_t eflags)
{
	asm volatile("pushl %0; popfl" : : "r" (eflags));
}

static inline uint32_t
read_ebp(void)
{
	uint32_t ebp;
	asm volatile("movl %%ebp,%0" : "=r" (ebp));
	return ebp;
}

static inline uint32_t
read_esp(void)
{
	uint32_t esp;
	asm volatile("movl %%esp,%0" : "=r" (esp));
	return esp;
}

static inline void
cpuid(uint32_t info, uint32_t *eaxp, uint32_t *ebxp, uint32_t *ecxp, uint32_t *edxp)
{
	uint32_t eax, ebx, ecx, edx;
	asm volatile("cpuid"
		     : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
		     : "a" (info));
	if (eaxp)
		*eaxp = eax;
	if (ebxp)
		*ebxp = ebx;
	if (ecxp)
		*ecxp = ecx;
	if (edxp)
		*edxp = edx;
}

static inline uint64_t
read_tsc(void)
{
	uint64_t tsc;
	asm volatile("rdtsc" : "=A" (tsc));
	return tsc;
}


// static 控制作用域，防止与其他同名函数冲突
// 避免重复定义: inline 函数的定义可能在多个编译单元中出现（特别是在头文件中）。如果不加 static，则可能会在链接阶段导致重复定义的问题
// 优化提示: inline 只是一个提示，编译器不一定会执行内联。而且在没有 static 的情况下，编译器可能会生成函数的实际定义，这可能与优化意图相悖。
// 内核环境的特殊性:可以减少函数调用的开销，确保这些函数的实现只出现在一个编译单元中

// 静态内联 xchg 函数 将 newval 的值与 addr 所指向的内存位置的值进行交换，并返回交换之前的旧值。
static inline uint32_t
xchg(volatile uint32_t *addr, uint32_t newval)
{
	uint32_t result;
	// lock 表示接下来的是原子操作
	// "xchgl %0, %1"：这是 xchg 指令，表示将 newval（即 %1 寄存器中的值）与 *addr（即 %0 内存地址中的值）交换。l 表示这是一个 32 位操作
	// "+m" (*addr)：+m 表示这是一个读-修改-写的操作，*addr 表示内存操作数。+ 意味着操作数既会被读取又会被修改。
	// "=a" (result)：=a 表示将操作的结果存储在 eax 寄存器中，并将该寄存器的值保存到 result 变量中。
	// "1" (newval)：1 表示这个操作数与前面第一个操作数（即 eax 寄存器）共享寄存器，并将 newval 的值加载到该寄存器中。
	// "cc"：表示汇编指令可能会修改 CPU 的状态标志（condition codes）。
	// The + in "+m" denotes a read-modify-write operand.
	asm volatile("lock; xchgl %0, %1"
		     : "+m" (*addr), "=a" (result)
		     : "1" (newval)
		     : "cc");
	return result;
}


// 如果你在 QEMU 中运行 JOS，可以通过向 QEMU 的特定 I/O 端口发送命令来触发关机
static inline void
qemu_poweroff(void) {
    // 0x2000 是 QEMU 关机的 I/O 端口，值为 0x1000 会触发关机
    outw(0x604, 0x2000);
}

// ACPI（Advanced Configuration and Power Interface）是现代计算机用来控制电源管理的标准。
// 可以通过向 ACPI 的电源管理寄存器发送命令来实现关机
static inline void
acpi_poweroff(void) {
    // ACPI 关机命令
    outw(0xB004, 0x2000);
}

// APM（Advanced Power Management）是早期的一种电源管理接口，可以通过向 BIOS 发出关闭命令来实现关机
static inline void
apm_poweroff(void) {
    asm volatile(
        "movw $0x5307, %ax\n"  // APM 关机命令
        "xorl %ebx, %ebx\n"
        "movw $0x0001, %bx\n"
        "int $0x15\n"
    );
}

// 在没有硬件支持或操作系统还没有配置 ACPI/APM 的情况下，可以触发三重故障（Triple Fault）来强制系统关闭。
// 三重故障通常会导致硬件自动重启或关机，这种方法在某些虚拟化环境下也适用：
static inline void
triple_fault_shutdown(void) {
    // 通过加载一个无效的全局描述符表 (GDT) 并触发 IDT 异常来引发三重故障
    lgdt(0);
    asm volatile("int $0x21");
}


#endif /* !JOS_INC_X86_H */
