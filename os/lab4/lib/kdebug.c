#include <kdebug.h>
#include <sbi.h>
#include <stdarg.h>
#include <string.h>

static unsigned long kpow(int x, int y);

void kputchar(int ch)
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
    kprintf("Panic at %s: %d\n", file, line);
    if (strlen(fmt)) {
        kprintf("Assert message: ");
        kprintf(fmt, ap);
    }
    kputchar('\n');
    va_end(ap);
    sbi_shutdown();
}

int kprintf(const char* fmt, ...)
{
    va_list ap;
    int val;
    int temp;
    char len;
    int rev = 0;
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
			case 'd':		//Decimal
				val = va_arg(ap, int);
				temp = val;
				len = 0;
				while (temp)
				{
					len++;
					temp /= 10;
				}
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
				val = va_arg(ap, int);
				temp = val;
				len = 0;
				while (temp)
				{
					len++;
					temp /= 16;
				}
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
			case 'c':		//character
				kputchar(va_arg(ap, int));
				rev += 1;
				break;
			default:
				break;
			}
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

static unsigned long kpow(int x, int y)
{
    unsigned long sum = 1;
    while (y--) {
        sum *= x;
    }
    return sum;
}
