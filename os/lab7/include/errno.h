#ifndef __ERRNO_H__
#define __ERRNO_H__
/**
 * @file errno.h
 * @brief 声明 errno 编号
 *
 * UNIX 系统调用失败时，返回码设为 -1,并根据失败原因
 * 设置 errno。
 *
 * 实际的 UNIX 系统中，每个线程都有一个 errno，errno 是
 * 一个可以展开为线程特定变量的宏。本内核没有实现线程，
 * 将 errno 简单的实现为全局变量。
 *
 * 本文件中的编号与最新的 Linux 内核一致。
 */
extern int errno;

#define    EPERM         1 /**< Operation not permitted */
#define    ENOENT         2 /**< No such file or directory */
#define    ESRCH         3 /**< No such process */
#define    EINTR         4 /**< Interrupted system call */
#define    EIO         5     /**< I/O error */
#define    ENXIO         6 /**< No such device or address */
#define    E2BIG         7 /**< Argument list too long */
#define    ENOEXEC         8 /**< Exec format error */
#define    EBADF         9 /**< Bad file number */
#define    ECHILD        10 /**< No child processes */
#define    EAGAIN        11 /**< Try again */
#define    ENOMEM        12 /**< Out of memory */
#define    EACCES        13 /**< Permission denied */
#define    EFAULT        14 /**< Bad address */
#define    ENOTBLK        15 /**< Block device required */
#define    EBUSY        16 /**< Device or resource busy */
#define    EEXIST        17 /**< File exists */
#define    EXDEV        18 /**< Cross-device link */
#define    ENODEV        19 /**< No such device */
#define    ENOTDIR        20 /**< Not a directory */
#define    EISDIR        21 /**< Is a directory */
#define    EINVAL        22 /**< Invalid argument */
#define    ENFILE        23 /**< File table overflow */
#define    EMFILE        24 /**< Too many open files */
#define    ENOTTY        25 /**< Not a typewriter */
#define    ETXTBSY        26 /**< Text file busy */
#define    EFBIG        27 /**< File too large */
#define    ENOSPC        28 /**< No space left on device */
#define    ESPIPE        29 /**< Illegal seek */
#define    EROFS        30 /**< Read-only file system */
#define ENOSYS      38 /**< Invalid system call number */
#define    ERESTART    85 /**< Interrupted system call should be restarted */

#endif /** end of include guard: __ERRNO_H__ */
