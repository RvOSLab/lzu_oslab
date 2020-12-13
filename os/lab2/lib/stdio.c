#include <stdio.h>
#include <kdebug.h>
#include <stdarg.h>


//stdarg.h定义的变量和宏：
//va_list
//适用于 va_start()、va_arg() 和 va_end() 这三个宏存储信息的类型

//va_start(ap, param)
//va_arg(ap, type)
//va_end(ap)
//va_copy(dest, src)*Linux用到，C标准中无

//va_list字符指针，指向当前参数，取参必须通过这个指针进行。
//va_start(ap, last_arg)对ap进行初始化，用于获取参数列表中可变参数的首指针
//ap用于保存函数参数列表中可变参数的首指针(即,可变参数列表)
//last_arg为函数参数列表中最后一个固定参数

//va_arg(ap, type)用于获取当前ap所指的可变参数并将并将ap指针移向下一可变参数
//输入参数ap(类型为va_list) : 可变参数列表，指向当前正要处理的可变参数
//输入参数type : 正要处理的可变参数的类型
//返回值 : 当前可变参数的值,是一个类型为 type 的表达式.

//va_end(ap)用于结束对可变参数的处理。
//实际上, va_end被定义为空.它只是为实现与va_start配对(实现代码对称和"代码自注释"功能)

static unsigned long kpow(int x, int y);

int kprintf(const char *_Format, ...) {
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
				str = va_arg(ap, const char *);
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
			break;
		case '\r':
			putchar('\r');
			break;
		case '\t':
			putchar('\t');
			break;
		default:
			putchar(*_Format);
		}
		_Format++;
	}
	va_end(ap);
	//return rev;
}

static unsigned long kpow(int x, int y) {
	unsigned long sum = 1;
	while (y--)
	{
		sum *= x;
	}
	return sum;
}

//TEST:
//int main() {
//	int a = 22;
//	char* p = "abc";
//	kprintf("%c%d",'c', 11);
//	kprintf("%x\r", a);
//	kprintf("%p", p);
//	kprintf("%s", "hi");
//	/*string乱码*/
//}