// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};


static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "GKL", "this function from gkl", mon_gkl },
	{ "backtrace", "Display ebp", mon_backtrace},
	{ "poweroff", "Shutting down", mon_poweroff}
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	if(argc >= 3){
		cprintf("more input!!! use like \" backtrace 12 \"\n");
	}

	// Your code here.
	uint32_t ebp = read_ebp();
	uint32_t eip;

	uint32_t num = 0;	
	if(argc == 2){
		char* s = argv[1];
		cprintf("input = \"%s\"\n", s);
		for(int i = 0; s[i] != '\0'; i++){
			num  =  num *10 + (s[i]-'0');
		}
	}


	struct Eipdebuginfo info;


	cprintf("Stack backtrace:\n");
	while (num-- && ebp != 0) {
		// eip 存储了被调用函数的返回地址
		eip = *((uint32_t*)ebp + 1); 
		if( debuginfo_eip( eip, &info) == -1 ){
			cprintf("read eip error!!\n");
		}
		cprintf("  ebp %08x  eip %08x  args", ebp, eip);
		for (int i = 2; i < 7; i++) {
			cprintf(" %08x", *(((uint32_t*)ebp) + i));
		}
		cprintf("\n");
		cprintf("\t %s:%d: %s+%d \r\n",info.eip_file, info.eip_line, info.eip_fn_name, info.eip_fn_namelen);

		ebp = *((uint32_t*)ebp);
	}
	return 0;
}

int
mon_gkl(int argc, char **argv, struct Trapframe *tf)
{
	if( argc != 3 ){
		cprintf("input num errors \n");
	}
	else{
		int a = (char)*argv[1]-'0';
		int b = (char)*argv[2]-'0';
		int sum =  a + b;
		cprintf(" %d + %d = %d \n", a, b, sum);
	}

	return 0;
}

int 
mon_poweroff(int argc, char **argv, struct Trapframe *tf)
{
    cprintf("Shutting down...\n");

    // 尝试 QEMU 关机接口
    qemu_poweroff();

    // // 尝试 ACPI 关机
    // acpi_poweroff();

    // // 尝试 APM 关机
    // apm_poweroff();

    // // 触发三重故障（最后的手段）
    // triple_fault_shutdown();

    // // 如果都失败，陷入死循环
    // while (1) {
    //     asm volatile("hlt");
    // }
	return 0;
}




/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;

	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor! \n");
	cprintf("Type 'help' for a list of commands.\n");
	cprintf("www this info form GKL HHHH !!! \n");



	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
