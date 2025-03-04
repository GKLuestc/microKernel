// Public definitions for the POSIX-like file descriptor emulation layer
// that our user-land support library implements for the use of applications.
// See the code in the lib directory for the implementation details.

#ifndef JOS_INC_FD_H
#define JOS_INC_FD_H

#include <inc/types.h>
#include <inc/fs.h>

// 结构体声明
struct Fd;
struct Stat;
struct Dev;

// Per-device-class file descriptor operations
// 设备描述符，设备结构体，需要绑定后使用
struct Dev {
	int dev_id;				// 设备的唯一标识符
	const char *dev_name;	// 设备名称
	ssize_t (*dev_read)(struct Fd *fd, void *buf, size_t len);			// 读取函数指针
	ssize_t (*dev_write)(struct Fd *fd, const void *buf, size_t len);	// 写入函数指针
	int (*dev_close)(struct Fd *fd);									// 关闭函数指针
	int (*dev_stat)(struct Fd *fd, struct Stat *stat);					// 获取设备状态
	int (*dev_trunc)(struct Fd *fd, off_t length);						// 截断设备中的数据，相当于设置文件大小
};


// 
struct FdFile {
	int id;
};


struct FdSock {
	int sockid;
};

// 文件描述符结构体
struct Fd {
	int fd_dev_id;		// 设备 ID
	off_t fd_offset;	// 当前偏移量
	int fd_omode;		// 打开模式（如只读，只写）
	union {
		// File server files
		// 文件服务器的文件描述符
		struct FdFile fd_file;
		// Network sockets
		struct FdSock fd_sock;
	};
};

// 文件状态结构体
struct Stat {
	char st_name[MAXNAMELEN];	// 文件名称
	off_t st_size;				// 文件大小
	int st_isdir;				// 是否是目录
	struct Dev *st_dev;			// 关联的设备
};


// 函数原型定义了一系列与文件描述符（Fd）和设备（Dev）相关的操作
// 由于这些操作对于不同的设备，都是通用的，所以提出出来

// 根据给定的文件描述符 fd 返回指向其数据的指针
char* fd2data(struct Fd *fd);

// 获取给定文件描述符 fd 的编号
int	fd2num(struct Fd *fd);

// 分配一个新的文件描述符，并将其存储在 fd_store 中
int	fd_alloc(struct Fd **fd_store);

// 关闭指定的文件描述符 fd,  must_exist参数指示如果文件描述符不存在是否应该返回错误。
int	fd_close(struct Fd *fd, bool must_exist);

// 根据文件描述符编号 fdnum 查找相应的文件描述符，并将结果存储在 fd_store 中
int	fd_lookup(int fdnum, struct Fd **fd_store);

// 根据设备 ID devid 查找相应的设备结构，并将结果存储在 dev_store 中
int	dev_lookup(int devid, struct Dev **dev_store);



extern struct Dev devfile;
extern struct Dev devsock;
extern struct Dev devcons;
extern struct Dev devpipe;

#endif	// not JOS_INC_FD_H
