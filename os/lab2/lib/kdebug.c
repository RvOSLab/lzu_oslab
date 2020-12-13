#include <kdebug.h>
#include <string.h>
#include <stdarg.h>

static unsigned long kpow(int x, int y);

void
kputchar(int ch)
{
    sbi_console_putchar(ch);
}

int
kputs(const char *msg)
{
    const char *ret = msg;
    for ( ; *msg != '\0'; ++msg )
    {
        kputchar(*msg);
    }
    kputchar('\n');
    return ret == msg;
}

void
do_panic(const char* file, int line, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    /// kprintf("--------------------------------------------------------------------------\n");
    /// kprintf("Panic at %s: %d\n", file, line);
    if (strlen(fmt))
    {
        /// kprintf("Assert message: ");
        /// kprintf(fmt, ap);
        kputs(fmt);
    }
    kputchar('\n');
    va_end(ap);
    sbi_shutdown();
    /// kprintf("Shutdown machine\n");
    /// kprintf("--------------------------------------------------------------------------\n");
}


int
kprintf(const char* _Format, ...) {
	va_list ap;		//定义可变参数的首指针
	int val;		//decimal value
	int temp;		//medium value （decimal to char）
	char len;		//decimal length
	int rev = 0;	//return value:length of string
	int ch;			//character
	const char* str = NULL;//string

	va_start(ap, _Format);
	while (*_Format != '\0')
	{
		switch (*_Format)
		{
		case '%':
			_Format++;
			switch (*_Format)
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
					putchar(ch + '0');				//putchar将数字转为字符输出
					len--;
				}
				break;
			case 'p':
				putchar('0');
				putchar('x');

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
						putchar(ch + '0');
					}
					else
					{
						putchar(ch - 10 + 'a');
					}
					len--;
				}
				break;

			case 's':		//string
				str = va_arg(ap, const char*);
				// rev += strlen(str);

				while (*str)
				{
					putchar(*str);
					str++;
				}
				break;
			case 'c':		//character
				putchar(va_arg(ap, int));
				rev += 1;
				break;
			default:
				break;
			}
		case '\n':
			putchar('\n');
			rev += 1;
			break;
		case '\r':
			putchar('\r');
			rev += 1;
			break;
		case '\t':
			putchar('\t');
			rev += 1;
			break;
		default:
			putchar(*_Format);
		}
		_Format++;
	}
	va_end(ap);
	return rev;
}

static unsigned long kpow(int x, int y) {
	unsigned long sum = 1;
	while (y--)
	{
		sum *= x;
	}
	return sum;
}
