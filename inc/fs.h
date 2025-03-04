// See COPYRIGHT for copyright information.

#ifndef JOS_INC_FS_H
#define JOS_INC_FS_H

#include <inc/types.h>
#include <inc/mmu.h>

// File nodes (both in-memory and on-disk)

// Bytes per file system block - same as page size
// 文件系统的块大小，一个页面大小
#define BLKSIZE		PGSIZE

// 块的位大小，即一个块有多少个bit位，所以是 * 8， 标志了一个bitmap能管理多少个块
#define BLKBITSIZE	(BLKSIZE * 8)

// Maximum size of a filename (a single path component), including null
// Must be a multiple of 4
// 最大文件名的长度
#define MAXNAMELEN	128

// Maximum size of a complete pathname, including null
// 文件路径名称的最大长度
#define MAXPATHLEN	1024

// Number of block pointers in a File descriptor
// 文件描述符中，直接块指针的数量，超出10个块(40KB)，就需要用到间接映射
#define NDIRECT		10

// Number of direct block pointers in an indirect block
// 一个间接块能存储的块指针数量，地址是32位即4B，故一个块最大能存储 4KB/4B = 1024个块的地址
#define NINDIRECT	(BLKSIZE / 4)

// 文件的最大大小，直接块 + 间接块 = （1024+10）* 4KB > 4MB
#define MAXFILESIZE	((NDIRECT + NINDIRECT) * BLKSIZE)


//文件描述符，文件结构体，用于文件系统的管理
struct File {
	char f_name[MAXNAMELEN];	// 文件名字 filename
	off_t f_size;				// 文件大小 file size in bytes
	uint32_t f_type;			// 文件类型 file type

	// Block pointers.
	// A block is allocated iff its value is != 0.
	uint32_t f_direct[NDIRECT];	// 直接块指针用于快速访问文件的前几个数据块。 direct blocks
	uint32_t f_indirect;		// 间接块指针 indirect block

	// Pad out to 256 bytes; must do arithmetic in case we're compiling
	// fsformat on a 64-bit machine.
	uint8_t f_pad[256 - MAXNAMELEN - 8 - 4*NDIRECT - 4]; //填充数组，用于确保结构体的大小为 256 字节。
} __attribute__((packed));	// required only on some 64-bit machines


// An inode block contains exactly BLKFILES 'struct File's
// 每个块中能够容纳的文件结构数量， 4KB / 256B = 16，一个块只能存储 16个文件描述符
#define BLKFILES	(BLKSIZE / sizeof(struct File))

// File types
// 文件类型
#define FTYPE_REG	0	// 普通文件 Regular file
#define FTYPE_DIR	1	// 目录文件 Directory


// File system super-block (both in-memory and on-disk)
// 文件系统魔数
#define FS_MAGIC	0x4A0530AE	// related vaguely to 'J\0S!'


// 超级块描述符，标志根目录，通常在磁盘的第 1 号块上
struct Super {
	uint32_t s_magic;		// 用于标识文件系统类型 Magic number: FS_MAGIC
	uint32_t s_nblocks;		// 总共块数大小 Total number of blocks on disk
	struct File s_root;		// 文件系统根目录的File描述结构体 Root directory node
};

// Definitions for requests from clients to file system
// 用户进程和 FS进程之间通信的请求标志
enum {
	FSREQ_OPEN = 1,		// 打开文件请求
	FSREQ_SET_SIZE,		// 设置文件大小请求

	// Read returns a Fsret_read on the request page
	FSREQ_READ,			// 读取文件请求
	FSREQ_WRITE,		// 写入文件请求

	// Stat returns a Fsret_stat on the request page
	FSREQ_STAT,			// 获取文件状态信息的请求
	FSREQ_FLUSH,		// 刷新文件缓冲区的请求
	FSREQ_REMOVE,		// 删除文件的请求
	FSREQ_SYNC			// 同步文件系统的请求
};

// 用户进程和 FS 进程相互通信的结构体（共用体），一个 page 大小
union Fsipc {

	// 打开文件的请求
	struct Fsreq_open {
		char req_path[MAXPATHLEN];	// 路径模式
		int req_omode;				// 打开模式
	} open;
	
	// 设置文件大小的请求
	struct Fsreq_set_size {
		int req_fileid;		// 文件 ID
		off_t req_size;		// 要设置的文件大小
	} set_size;


	// 读取文件的请求
	struct Fsreq_read {
		int req_fileid;		// 文件 ID
		size_t req_n;		// 要读取的字节数
	} read;

	// FS 进程处理读取请求之后，返回的数据缓冲区，用来存储读取的数据
	struct Fsret_read {
		char ret_buf[PGSIZE];	// 读取数据的缓冲区，最大一个页面 4KB
	} readRet;


	// 写入文件的请求
	struct Fsreq_write {
		int req_fileid;		// 文件 ID
		size_t req_n;		// 要写入的字节数
		char req_buf[PGSIZE - (sizeof(int) + sizeof(size_t))];	// 要写入的数据，减去两个标志量的大小，保持最大一个页面
	} write;


	// 获取文件状态的请求
	struct Fsreq_stat {
		int req_fileid;		//文件 ID
	} stat;

	// FS 进程处理 “获取文件状态请求” 后返回的结构体，
	struct Fsret_stat {
		char ret_name[MAXNAMELEN];	// 文件名字
		off_t ret_size;				// 文件大小
		int ret_isdir;				// 是否为目录
	} statRet;


	// 刷新文件的请求
	struct Fsreq_flush {
		int req_fileid;		// 文件 ID
	} flush;

	// 删除文件的请求
	struct Fsreq_remove {
		char req_path[MAXPATHLEN];	// 文件路径
	} remove;

	// 确保 Fsipc 结构体的大小为一个页面
	// Ensure Fsipc is one page
	char _pad[PGSIZE];
};

#endif /* !JOS_INC_FS_H */
