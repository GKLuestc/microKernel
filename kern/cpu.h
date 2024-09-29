
#ifndef JOS_INC_CPU_H
#define JOS_INC_CPU_H

#include <inc/types.h>
#include <inc/memlayout.h>
#include <inc/mmu.h>
#include <inc/env.h>

// Maximum number of CPUs
#define NCPU  8

// Values of status in struct Cpu
enum {
	CPU_UNUSED = 0,		//表示这个 CPU 目前未被使用或尚未初始化。
	CPU_STARTED,		//表示该 CPU 已经被启动并运行
	CPU_HALTED,			//表示该 CPU 处于暂停或停止状态。
};

// 每个 CPU 在多处理器系统中的状态和相关信息 Per-CPU state
struct CpuInfo {
	uint8_t cpu_id;                 // 本地 APIC 的 ID，它是一个唯一标识符，用于标识系统中的每个 CPU（或核心） 	Local APIC ID; index into cpus[] below
	volatile unsigned cpu_status;   // 当前 CPU 的状态，用于标识该 CPU 的运行状态。 						The status of the CPU
	struct Env *cpu_env;            // 一个指向 Env 结构体的指针，用于指向当前正在此 CPU 上运行的执行环境		The currently-running environment.
	struct Taskstate cpu_ts;        //这个字段用于保存 x86 架构中的任务状态段 								 Used by x86 to find stack for interrupt
};

// Initialized in mpconfig.c
extern struct CpuInfo cpus[NCPU];
extern int ncpu;                    // Total number of CPUs in the system
extern struct CpuInfo *bootcpu;     // The boot-strap processor (BSP)
extern physaddr_t lapicaddr;        // Physical MMIO address of the local APIC

// Per-CPU kernel stacks
extern unsigned char percpu_kstacks[NCPU][KSTKSIZE];

int cpunum(void);
#define thiscpu (&cpus[cpunum()])

void mp_init(void);
void lapic_init(void);
void lapic_startap(uint8_t apicid, uint32_t addr);
void lapic_eoi(void);
void lapic_ipi(int vector);

#endif
