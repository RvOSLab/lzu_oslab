#include <sbi.h>
#include <kdebug.h>
#include <mm.h>
#include <trap.h>
#include <sched.h>
#include <clock.h>
#include <syscall.h>
#include <device/loader.h>
#include <device/block/block_cache.h>
#include <fs/vfs.h>

int main(const char* args, const struct fdt_header *fdt)
{
    kputs("\nLZU OS STARTING....................");
    print_system_infomation();
    mem_init();
    mem_test();
    malloc_test();
    init_device_table();
    init_mem_resource();
    fdt_loader(fdt, driver_list);
    set_stvec();
    sched_init();
    clock_init();
    block_cache_init();
    vfs_init();
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
        
        char buffer[64];
        int buffer_p;
        while (1) {
            puts("\033[1;32mroot@lzuoslab\033[0m:");
            puts("\033[1;34m"); puts("test"); puts("\033[0m"); 
            puts("$ ");
            buffer_p = 0;                       
            while (1) {
                buffer[buffer_p] = get_char();
                if (buffer[buffer_p] == '\x03') {
                    puts("^C");
                    put_char('\n');
                    buffer[0] = '\0';
                    break;
                }
                if (buffer[buffer_p] == '\e') {
                    buffer[buffer_p] = '^';
                    put_char(buffer[buffer_p]);
                    buffer_p += 1;
                    buffer[buffer_p] = '[';
                }
                if (buffer[buffer_p] == '\x7f') {
                    if (buffer_p) {
                        buffer_p -= 1;
                        puts("\b\e[K");
                    }
                    continue;
                }
                if (buffer[buffer_p] == '\r') {
                    buffer[buffer_p] = '\0';
                    put_char('\n');
                    break;
                }
                put_char(buffer[buffer_p]);
                buffer_p++;
            }
            char *buffer_nospace = buffer;
            while(*buffer_nospace && (*buffer_nospace == ' ' || *buffer_nospace == '\t')) {
                buffer_nospace += 1;
            }
            char *arg1 = (char *)strchr(buffer_nospace, ' ');
            if (arg1) {
                *arg1 = '\0';
                arg1 += 1;
            }
            if (!strcmp(buffer_nospace, "t")) {
                // syscall(NR_test_net);
                int fd = syscall(NR_open, "/test/test.txt");
                const char *str_write = "write test\n";
                syscall(NR_seek, fd, 7, 1); // define SEEK_SET 1
                syscall(NR_write, fd, str_write, 12);
                syscall(NR_close, fd);
                puts("write 'write test\\n' > /test/test.txt@7\n");
            } else if (!strcmp(buffer_nospace, "b")) {
                syscall(NR_block);
            } else if (!strcmp(buffer_nospace, "q")) {
                syscall(NR_reset, 0);   // #define SHUTDOWN_FUNCTION 0
            } else if (!strcmp(buffer_nospace, "r")) {
                syscall(NR_reset, 1);   // #define REBOOT_FUNCTION 1
            } else {
                if (!strcmp(buffer_nospace, "cat")) {
                    if (!arg1 || !strlen(arg1)) {
                        puts("Usage: cat [FILE]\n");
                        continue;
                    }
                    int fd = syscall(NR_open, arg1);
                    if (fd == -1) {
                        puts("cat: "); puts(arg1); puts(": No such file or directory\n");
                        continue;
                    }
                    struct vfs_stat stat;
                    syscall(NR_stat, fd, &stat);
                    char file_buffer[512];
                    if (stat.size > 512) stat.size = 512;
                    syscall(NR_read, fd, file_buffer, stat.size);
                    puts(arg1); puts(": "); puts(file_buffer);
                    syscall(NR_close, fd);
                    continue;
                }
                if (buffer_nospace[0]) {
                    puts(buffer_nospace); puts(": command not found\n");
                }
            }
        }
    }
    while (1)
        ; /* infinite loop */
    return 0;
}
