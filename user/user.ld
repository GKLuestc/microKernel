/* Simple linker script for JOS user-level programs.
   See the GNU ld 'info' manual ("info ld") to learn the syntax. */

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
	/* Load programs at this address: "." means the current address */
	. = 0x800020;

	/* 定义了代码段，包括程序的可执行代码部分。.text 段通常包含所有的函数和方法定义。 */
	.text : {
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}

	PROVIDE(etext = .);	/* 用于定义一个名为 etext 的符号，该符号的值是 .text 段结束时的地址。 Define the 'etext' symbol to this value */

	/* 定义了只读数据段。通常包含常量数据，如字符串常量和其他不可修改的数据。 */
	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	/* Adjust the address for the data segment to the next page */
	/* 在加载 .data 段之前，地址会被调整到下一个页对齐处（ALIGN(0x1000)），以确保内存布局的对齐性。*/
	. = ALIGN(0x1000);

	/* 包含了可读写的初始化数据段。这里面存储了程序中已初始化的全局变量。 */
	.data : {
		*(.data)
	}

	/* 用于定义一个名为 edata 的符号，该符号的值是 .data 段结束时的地址。 */
	PROVIDE(edata = .);

	/* 包含未初始化的全局变量和静态变量。程序启动时，这些变量会被自动清零 */
	.bss : {
		*(.bss)
	}

	/* 用于定义一个名为 end 的符号，该符号的值是 .bss 段结束时的地址。 */
	PROVIDE(end = .);


	/* Place debugging symbols so that they can be found by
	 * the kernel debugger.
	 * Specifically, the four words at 0x200000 mark the beginning of
	 * the stabs, the end of the stabs, the beginning of the stabs
	 * string table, and the end of the stabs string table, respectively.
	 * 用于存放调试信息，特别是 stabs 格式的调试符号和字符串表。
	 	.stab_info 段是一个特殊的段，存放在物理地址 0x200000 处，用于记录调试符号和字符串表的起止地址。
		.stab 段存放实际的调试符号数据。
		.stabstr 段存放调试符号对应的字符串数据。
	 */

	.stab_info 0x200000 : {
		LONG(__STAB_BEGIN__);
		LONG(__STAB_END__);
		LONG(__STABSTR_BEGIN__);
		LONG(__STABSTR_END__);
	}

	.stab : {
		__STAB_BEGIN__ = DEFINED(__STAB_BEGIN__) ? __STAB_BEGIN__ : .;
		*(.stab);
		__STAB_END__ = DEFINED(__STAB_END__) ? __STAB_END__ : .;
		BYTE(0)		/* Force the linker to allocate space
				   for this section */
	}

	.stabstr : {
		__STABSTR_BEGIN__ = DEFINED(__STABSTR_BEGIN__) ? __STABSTR_BEGIN__ : .;
		*(.stabstr);
		__STABSTR_END__ = DEFINED(__STABSTR_END__) ? __STABSTR_END__ : .;
		BYTE(0)		/* Force the linker to allocate space
				   for this section */
	}

	/* 这个段是用来丢弃一些不需要的部分，例如 
		.eh_frame（异常处理相关数据）、
		.note.GNU-stack（表示栈是否可执行）和 .comment（编译器的注释）段。 
	*/
	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack .comment)
	}
}
