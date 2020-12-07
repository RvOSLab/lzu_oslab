/**
 * @file assert.h
 * 本文件实现 assert() 和 panic()
 */
#include <stdarg.h>
#include <stdio.h>

inline void
do_panic(const char* file, int line, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf("--------------------------------------------------------------------------\n");
    printf("Panic at %s: %d\n", file, line);
    if (strlen(fmt))
    {
        printf("Assert message: ");
        printf(fmt, ap);
    }
    putchar('\n');
    va_end(ap);
    printf("Shutdown machine\n");
    printf("--------------------------------------------------------------------------\n");
}

#define GET_ARRAY_CONTENT_IMP(...) __VA_ARGS__
#define GET_ARRAY_CONTENT(array) "" GET_ARRAY_CONTENT##_IMP array
#define __do_panic(array) do_panic(__FILE__, __LINE__, GET_ARRAY_CONTENT(array))

/*******************************************************************************
 * @brief 终止程序
 *
 * 这个宏是可变参数宏，可以不加参数地调用（直接终止程序）
 * 也可以加参数当成 printf() 使用（终止程序并打印信息)。
 * 例子：
 * ```
 *        panic();
 *        panic("Hello young programmer.");
 *        panic("Correct value: %d", 100);
 * ```
 *
 * @see printf(), assert()
 ******************************************************************************/
#define panic(...) __do_panic((__VA_ARGS__))

/*******************************************************************************
 * @brief 断言宏
 *
 * @param cond 条件表达式 当 cond 为假时，终止程序。
 *
 * 这个宏是可变参数宏，可以当成 printf() 使用。
 * 例子：
 * ```
 *      assert(i == 100);
 *      assert(i == 100, "i should be 100");
 *      assert(i == 100, "i should be %d", 100);
 * ```
 ******************************************************************************/
#define assert(cond, ...)                                                      \
    do {                                                                       \
        if (!(cond)) {                                                         \
            panic(__VA_ARGS__);                                                \
        }                                                                      \
    } while (0)
