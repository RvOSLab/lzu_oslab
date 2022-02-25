#include <sbi.h>
#include <kdebug.h>
#include <mm.h>
#include <trap.h>
#include <sched.h>
#include <clock.h>
#include <syscall.h>
#include <device/loader.h>
#include <fs/vfs.h>

int main(const char* args, const struct fdt_header *fdt)
{
    kputs("\nLZU OS STARTING....................");
    print_system_infomation();
    mem_init();
    mem_test();
    malloc_test();
    init_device_table();
    fdt_loader(fdt, driver_list);
    set_stvec();
    vfs_init();
    sched_init();
    clock_init();
    kputs("Hello LZU OS");
    enable_interrupt();
    init_task0();

    syscall(NR_fork);    /* task 0 creates task 1 */
    long local = syscall(NR_getpid);
    syscall(NR_test_fork, local);
    if (local) {
        #define get_char() ((uint8_t) syscall(NR_char, 0))
        #define put_char(c) (syscall(NR_char, (uint64_t) (c)))
        #define puts(str) do { \
            for (char *cp = str; *cp; cp++) put_char(*cp); \
        } while (0)

        int fd = syscall(NR_open, "/test.txt");
        struct vfs_stat stat;
        syscall(NR_stat, fd, &stat);
        char file_buffer[64];
        syscall(NR_read, fd, file_buffer, stat.size);
        puts("/test.txt: "); puts(file_buffer);
        syscall(NR_close, fd);
        
        char buffer[64];
        int buffer_p;
        while (1) {
            puts("\033[1;32mroot@lzuoslab\033[0m:");
            puts("\033[1;34m"); puts("test"); puts("\033[0m"); 
            puts("$ ");
            buffer_p = 0;                       
            while (1) {
                buffer[buffer_p] = get_char();
                put_char(buffer[buffer_p]);
                if (buffer[buffer_p] == '\r') {
                    buffer[buffer_p] = '\n';
                    put_char(buffer[buffer_p]);
                    buffer[buffer_p] = '\0';
                    buffer_p = 0;
                    break;
                }
                buffer_p++;
            }
            if (buffer[0] == 't' && !buffer[1]) {
                puts("char dev test\n");
            } else if (buffer[0] == 'b' && !buffer[1]) {
                syscall(NR_block);
            } else if (buffer[0] == 'q' && !buffer[1]) {
                syscall(NR_reset, 0);   // #define SHUTDOWN_FUNCTION 0
            } else if (buffer[0] == 'r' && !buffer[1]) {
                syscall(NR_reset, 1);   // #define REBOOT_FUNCTION 1
            } else {
                if (buffer[0]) {
                    puts(buffer); puts(": command not found\n");
                }
            }
        }
    }
    while (1)
        ; /* infinite loop */
    return 0;
}
