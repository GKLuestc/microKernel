/* See COPYRIGHT for copyright information. */

#ifndef JOS_INC_ENV_H
#define JOS_INC_ENV_H

#include <inc/types.h>
#include <inc/trap.h>
#include <inc/memlayout.h>

typedef int32_t envid_t;

// An environment ID 'envid_t' has three parts:
//	环境的 id 由以下三部分组成
// +1+---------------21-----------------+--------10--------+
// |0|          Uniqueifier             |   Environment    |
// | |                                  |      Index       |
// +------------------------------------+------------------+
//                                       \--- ENVX(eid) --/
//
// The environment index ENVX(eid) equals the environment's index in the
// 'envs[]' array.  The uniqueifier distinguishes environments that were
// created at different times, but share the same environment index.
//
// All real environments are greater than 0 (so the sign bit is zero).
// envid_ts less than 0 signify errors.  The envid_t == 0 is special, and
// stands for the current environment.

#define LOG2NENV		10
#define NENV			(1 << LOG2NENV)		// 总共的环境数量，1024个环境
#define ENVX(envid)		((envid) & (NENV - 1))  // 环境 id 的低10位表示在 env数组中的位置

// Values of env_status in struct Env
// 						  运行态
//						//   	 \
//		新空间 ---> 就绪态  <---  阻塞态
enum {
	ENV_FREE = 0,		//表示该进程（环境）当前未被使用，即该环境结构体处于空闲状态。
	ENV_DYING,			//表示该进程正在终止，但还没有完全释放资源。
	ENV_RUNNABLE,		//表示该进程是可运行的，即它可以被调度运行。
	ENV_RUNNING,		//表示该进程正在运行中。
	ENV_NOT_RUNNABLE	//表示该进程当前不可运行，即它处于阻塞状态。
};

// 特殊环境类型，用户环境，fs进程环境
// Special environment types
enum EnvType {
	ENV_TYPE_USER = 0,
	ENV_TYPE_FS,		// File system server
};

struct Env {
	struct Trapframe env_tf;	// 在 inc/trap.h 中定义的这个结构体，在该环境不运行时保存该环境的寄存器值 Saved registers
	struct Env *env_link;		// 这是一个指向 env_free_list 中的下一个 Env 的指针 Next free Env
	envid_t env_id;				// 内核在这里存储一个值，该值唯一标识当前使用这个Env结构的环境 Unique environment identifier
	envid_t env_parent_id;		// 内核在这里存储创建该环境的环境（类似于父进程）的env_id env_id of this env's parent
	enum EnvType env_type;		// 这个是用来区分特殊环境的。大多数环境都是ENV_TYPE_USER类型。 Indicates special system environments
	unsigned env_status;		// 当前环境的状态 Status of the environment
	uint32_t env_runs;			// 记录运行的次数，可用于统计和调试 Number of times environment has run
	int env_cpunum;				// The CPU that the env is running on

	// Address space
	pde_t *env_pgdir;		// 指向该环境的页目录，管理该环境的虚拟地址空间。Kernel virtual address of page dir

	// Exception handling
	void *env_pgfault_upcall;	// 用户环境，为自己注册一个页面异常处理函数 Page fault upcall entry point

	// Lab 4 IPC
	bool env_ipc_recving;		// 是否需要接收数据，0 - 不需要， 1 - 需要      Env is blocked receiving
	void *env_ipc_dstva;		// 当前环境接收页面数据映射到的虚拟地址				VA at which to map received page
	uint32_t env_ipc_value;		// 接收到的一个数据							  Data value sent to us
	envid_t env_ipc_from;		// 接收到的数据来源							  envid of the sender
	int env_ipc_perm;			// 接收的页面权限								 Perm of page mapping received
};

#endif // !JOS_INC_ENV_H
