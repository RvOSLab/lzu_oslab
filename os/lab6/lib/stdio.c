#ifdef COMPILE
#include <lib/stdio.h>
#include <stddef.h>
static uint64_t pow(uint64_t x, uint64_t y);

int printf(const char* fmt, ...)
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
                    ch = temp / pow(10, len - 1);
                    temp %= pow(10, len - 1);
                    put_char(ch + '0');
                    len--;
                }
                break;
            case 'p':
                put_char('0');
                put_char('x');
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
                    ch = temp / pow(16, len - 1);
                    temp %= pow(16, len - 1);
                    if (ch <= 9)
                    {
                        put_char(ch + '0');
                    }
                    else
                    {
                        put_char(ch - 10 + 'a');
                    }
                    len--;
                }
                break;
            case 's':
                str = va_arg(ap, const char*);

                while (*str)
                {
                    put_char(*str);
                    str++;
                }
                break;
            case 'c':        //character
                put_char(va_arg(ap, int));
                rev += 1;
                break;
            default:
                break;
            }
            break;
        case '\n':
            put_char('\n');
            rev += 1;
            break;
        case '\r':
            put_char('\r');
            rev += 1;
            break;
        case '\t':
            put_char('\t');
            rev += 1;
            break;
        default:
            put_char(*fmt);
        }
        fmt++;
    }
    va_end(ap);
    return rev;
}

static uint64_t pow(uint64_t x, uint64_t y)
{
    uint64_t sum = 1;
    while (y--) {
        sum *= x;
    }
    return sum;
}
#endif
