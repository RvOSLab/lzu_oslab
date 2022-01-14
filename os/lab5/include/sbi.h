#ifndef __SBI_H__
#define __SBI_H__
#include <stddef.h>

/** error type */
#define SBI_SUCCESS 0
#define SBI_ERR_FAILED -1
#define SBI_ERR_NOT_SUPPORTED -2
#define SBI_ERR_INVALID_PARAM -3
#define SBI_ERR_DENIED -4
#define SBI_ERR_INVALID_ADDRESS -5
#define SBI_ERR_ALERADY_AVAILABLE -6

/** extenstion id */
#define BASE_EXTENSTION 0x10
#define TIMER_EXTENTION 0x54494D45
#define HART_STATE_EXTENTION 0x48534D
#define RESET_EXTENTION 0x53525354

/** sbi implementation id */

/** we only use OpenSBI */
#define BERKELY_BOOT_LOADER 0
#define OPENSBI 1
#define XVISOR 2
#define KVM 3
#define RUSTSBI 4
#define DIOSIX 5

/** sbi ecall return type */
struct sbiret {
    long error;
    long value;
};

struct sbiret sbi_get_spec_version();                 /** get sbi specification version */
struct sbiret sbi_get_impl_id();                      /** get sbi implementation id */
struct sbiret sbi_get_impl_version();                 /** get sbi implementation version */
struct sbiret sbi_probe_extension(long extension_id); /** probe sbi extenstion */
void sbi_set_timer(uint64_t stime_value);             /** set timer */
char sbi_console_getchar();                            /** read a byte from debug console */
void sbi_console_putchar(char ch);                     /** print character to debug console */
void sbi_shutdown();                                  /** shutdown */
void print_system_infomation();

#endif
