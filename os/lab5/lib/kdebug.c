#include <kdebug.h>
#include <sbi.h>
#include <stdarg.h>
#include <string.h>

static uint64_t kpow(uint64_t x, uint64_t y);

void kputchar(char ch)
{
    sbi_console_putchar(ch);
}

int kputs(const char* msg)
{
    const char* ret = msg;
    for (; *msg != '\0'; ++msg) {
        kputchar(*msg);
    }
    kputchar('\n');
    return ret == msg;
}

void do_panic(const char* file, int line, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    kprintf("--------------------------------------------------------------------------\n");
    kprintf("Panic at %s: %u\n", file, line);
    if (strlen(fmt)) {
        kprintf("Assert message: ");
        kprintf(fmt, ap);
    }
    kputchar('\n');
    va_end(ap);
    kprintf("Shutdown machine\n");
    kprintf("--------------------------------------------------------------------------\n");
    sbi_shutdown();
}

int kprintf(const char* fmt, ...)
{
    va_list ap;
    uint64_t val;
    uint64_t temp;
    uint64_t len;
    uint64_t rev = 0;
    int ch;
    const char* str = NULL;

    va_start(ap, fmt);
    while (*fmt != '\0')
    {
        switch (*fmt)
        {
        case '%':
            fmt++;
            switch (*fmt)
            {
            case 'u':        //Decimal
                val = va_arg(ap, uint64_t);
                temp = val;
                len = 0;
                do
                {
                    len++;
                    temp /= 10;
                } while (temp);
                rev += len;
                temp = val;
                while (len)
                {
                    ch = temp / kpow(10, len - 1);
                    temp %= kpow(10, len - 1);
                    kputchar(ch + '0');
                    len--;
                }
                break;
            case 'p':
                kputchar('0');
                kputchar('x');
            case 'x':
                val = va_arg(ap, uint64_t);
                temp = val;
                len = 0;
                do
                {
                    len++;
                    temp /= 16;
                } while (temp);
                rev += len;
                temp = val;
                while (len)
                {
                    ch = temp / kpow(16, len - 1);
                    temp %= kpow(16, len - 1);
                    if (ch <= 9)
                    {
                        kputchar(ch + '0');
                    }
                    else
                    {
                        kputchar(ch - 10 + 'a');
                    }
                    len--;
                }
                break;
            case 's':
                str = va_arg(ap, const char*);

                while (*str)
                {
                    kputchar(*str);
                    str++;
                }
                break;
            case 'c':        //character
                kputchar(va_arg(ap, int));
                rev += 1;
                break;
            default:
                break;
            }
            break;
        case '\n':
            kputchar('\n');
            rev += 1;
            break;
        case '\r':
            kputchar('\r');
            rev += 1;
            break;
        case '\t':
            kputchar('\t');
            rev += 1;
            break;
        default:
            kputchar(*fmt);
        }
        fmt++;
    }
    va_end(ap);
    return rev;
}

static uint64_t kpow(uint64_t x, uint64_t y)
{
    uint64_t sum = 1;
    while (y--) {
        sum *= x;
    }
    return sum;
}
