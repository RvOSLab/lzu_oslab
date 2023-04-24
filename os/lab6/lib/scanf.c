#ifdef COMPILE
#include <lib/stdio.h>

/* 状态机相关宏定义 */
#define STATE_MACHINE(state_var) switch (state_var) {
#define END_STATE_MACHINE() \
    default:                                   \
        break;                                 \
    }

#define STATE(state) case (state):
#define END_STATE() break;
/* 只能在状态机里的switch里使用 */
#define KEEP_STATE() break;
#define SWITCH_STATE(state_var, state) { \
    (state_var) = (state);               \
    break;                               \
}

/* 格式化字符串相关宏定义 */
enum format_parse_state {
    NORMAL_CHAR = 0,
    FORMAT_PARSE,
    FORMAT_PARSE_WIDTH,
    FORMAT_PARSE_FORMAT
};

#define format_end_parse() { \
    va_end(args);      \
    return used_char_num;   \
}
#define format_consume_char() { format_str_ptr += 1; }

#define format_get_num() ({ \
    int tmp = 0;       \
    while(1) {         \
        if (*format_str_ptr > '9' || *format_str_ptr < '0') { \
            break;     \
        }              \
        tmp *= 10;     \
        tmp += *format_str_ptr - '0'; \
        format_consume_char(); \
    }                  \
    tmp;               \
})

#define format_get_integer_width() ({ \
    int tmp_width;                    \
    if (*format_str_ptr == 'l') {     \
        tmp_width = 4;                \
        format_consume_char();        \
        if (*format_str_ptr == 'l') { \
            tmp_width = 8;            \
            format_consume_char();    \
        }                             \
    } else {                          \
        tmp_width = 2;                \
        format_consume_char();        \
        if (*format_str_ptr == 'h') { \
            tmp_width = 1;            \
            format_consume_char();    \
        }                             \
    }                                 \
    tmp_width;                        \
})

/* 输入相关宏定义 */
#define input_read_char() ({ \
    if (input_next_char == 0x100) {        \
        input_next_char = get_char();     \
    }                                      \
    (char)input_next_char;                 \
})
#define input_consume_char() { \
    input_next_char = 0x100;               \
    used_char_num += 1;                    \
}

#define input_put_integer(int_type, val) { \
    if (!skip_set_ptr) {                   \
        int_type *tmp_int = va_arg(args, int_type *); \
        *tmp_int = (int_type) (val);       \
    }                                      \
}

#define input_put_n() { input_put_integer(int, used_char_num); }

#define input_put_integer_by_width(val, is_unsigned) { \
    if (is_unsigned) {                                          \
        switch (integer_width) {                                \
            case 0:                                             \
                input_put_integer(unsigned int, val);           \
                break;                                          \
            case 1:                                             \
                input_put_integer(unsigned char, val);          \
                break;                                          \
            case 2:                                             \
                input_put_integer(unsigned short int, val);     \
                break;                                          \
            case 4:                                             \
                input_put_integer(unsigned long int, val);      \
                break;                                          \
            case 8:                                             \
                input_put_integer(unsigned long long int, val); \
                break;                                          \
        }                                                       \
    } else {                                                    \
        switch (integer_width) {                                \
            case 0:                                             \
                input_put_integer(int, val);                    \
                break;                                          \
            case 1:                                             \
                input_put_integer(char, val);                   \
                break;                                          \
            case 2:                                             \
                input_put_integer(short int, val);              \
                break;                                          \
            case 4:                                             \
                input_put_integer(long int, val);               \
                break;                                          \
            case 8:                                             \
                input_put_integer(long long int, val);          \
                break;                                          \
        }                                                       \
    }                                                           \
}


#define input_to_integer(is_unsigned, end_char) { \
    int read_char_num = 0;                 \
    int tmp = 0;                           \
    int is_neg = 0;                        \
    char tmp_char = input_read_char();     \
    if (tmp_char == '+' || tmp_char == '-') { \
        is_neg = tmp_char == '-';          \
        input_consume_char();              \
        read_char_num = 1;                 \
    }                                      \
    tmp_char = input_read_char();          \
    if (tmp_char < '0' || tmp_char > end_char) {\
        format_end_parse();                \
    }                                      \
    while (1) {                            \
        tmp_char = input_read_char();      \
        if (tmp_char < '0' || tmp_char > end_char) { \
            break;                         \
        }                                  \
        input_consume_char();              \
        read_char_num += 1;                \
        tmp *= end_char - '0' + 1;         \
        tmp += tmp_char - '0';             \
        if (max_read_char_num && max_read_char_num == read_char_num) { \
            break;                         \
        }                                  \
    }                                      \
    tmp = is_neg ? -tmp : tmp;             \
    input_put_integer_by_width(tmp, is_unsigned); \
}

#define input_to_hex(check_symbol) { \
    int read_char_num = 0;                 \
    int tmp = 0;                           \
    int is_neg = 0;                        \
    char tmp_char = input_read_char();     \
    if (check_symbol) {                    \
        if (tmp_char == '+' || tmp_char == '-') { \
            is_neg = tmp_char == '-';      \
            input_consume_char();          \
            read_char_num += 1;            \
        }                                  \
        tmp_char = input_read_char();      \
    }                                      \
    if (tmp_char < '0' || tmp_char > '9') {\
        tmp_char |= 0b100000;              \
        if (tmp_char < 'a' || tmp_char > 'f') { \
            format_end_parse();            \
        }                                  \
    }                                      \
    while (1) {                            \
        tmp_char = input_read_char();      \
        if (tmp_char < '0' || tmp_char > '9') { \
            tmp_char |= 0b100000;          \
            if (tmp_char < 'a' || tmp_char > 'f') { \
                break;                     \
            }                              \
            tmp_char += '9' + 1 - 'a';     \
        }                                  \
        input_consume_char();              \
        read_char_num += 1;                \
        tmp *= 16;                         \
        tmp += tmp_char - '0';             \
        if (max_read_char_num && max_read_char_num == read_char_num) { \
            break;                         \
        }                                  \
    }                                      \
    tmp = is_neg ? -tmp : tmp;             \
    input_put_integer_by_width(tmp, 0);    \
}

#define input_to_pointer() { \
    if (max_read_char_num && max_read_char_num < 3) { \
        format_end_parse();                \
    }                                      \
    char temp_head_char = input_read_char(); \
    if (temp_head_char != '0') {           \
        format_end_parse();                \
    }                                      \
    input_consume_char();                  \
    temp_head_char = input_read_char();    \
    if (temp_head_char != 'x' && temp_head_char != 'X') { \
        format_end_parse();                \
    }                                      \
    input_consume_char();                  \
    input_to_hex(0);                       \
}

#define input_skip_space() { \
    while(1) {                             \
        char next_char = input_read_char();\
        if (next_char != '\t' && next_char != '\n' && next_char != ' ') { \
            break;                         \
        }                                  \
        input_consume_char();              \
    }                                      \
}

#define input_to_str(space_break) { \
    int read_char_num = 0;                 \
    char *char_ptr;                        \
    if (!skip_set_ptr) char_ptr = va_arg(args, char *); \
    while(1) {                             \
        char tmp = input_read_char();      \
        input_consume_char();              \
        read_char_num += 1;                \
        if (space_break && (tmp == '\t' || tmp == '\n' || tmp == ' ')) { \
            break;                         \
        }                                  \
        if (!skip_set_ptr) {               \
            *char_ptr = tmp;               \
            char_ptr += 1;                 \
        }                                  \
        if (max_read_char_num && max_read_char_num == read_char_num) { \
            break;                         \
        }                                  \
    }                                      \
    if (!skip_set_ptr) {                   \
        *char_ptr = '\0';                  \
    }                                      \
}

int scanf(const char *format_str, ...) {
    /* 初始化参数列表 */
    va_list args;
    va_start(args, format_str);

    /* 状态机所需的参数 */
    const char *format_str_ptr = format_str;
    int used_char_num = 0;
    int parse_state = NORMAL_CHAR;

    /* 输入缓冲区(0x100表示缓冲区为空) */
    int input_next_char = 0x100;

    /* 格式化参数 */
    int skip_set_ptr = 0;      // 没有对应的指针保存数据
    int max_read_char_num = 0; // 某个类型最多读取的字符数
    int integer_width = 0;     // 整数宽度

    /* 循环过程 */
    while(1) {
        STATE_MACHINE(parse_state)
        STATE(NORMAL_CHAR) {
            switch (*format_str_ptr) {
                case '\0':
                    format_end_parse();
                case '%':
                    SWITCH_STATE(parse_state, FORMAT_PARSE);
                case ' ': case '\t': case '\n':
                    input_skip_space();
                    KEEP_STATE();
                default:
                    if (input_read_char() != *format_str_ptr) {
                        format_end_parse();
                    }
                    input_consume_char();
                    KEEP_STATE();
            }
            format_consume_char();
            END_STATE();
        }
        STATE(FORMAT_PARSE) {
            switch (*format_str_ptr) {
                case '*':
                    skip_set_ptr = 1;
                    format_consume_char();
                    KEEP_STATE();
                case '0' ... '9':
                    max_read_char_num = format_get_num();
                    SWITCH_STATE(parse_state, FORMAT_PARSE_WIDTH);
                default:
                    SWITCH_STATE(parse_state, FORMAT_PARSE_WIDTH);
            }
            END_STATE();
        }
        STATE(FORMAT_PARSE_WIDTH) {
            switch (*format_str_ptr) {
                case 'h': case 'l':
                    integer_width = format_get_integer_width();
                    SWITCH_STATE(parse_state, FORMAT_PARSE_FORMAT);
                default:
                    SWITCH_STATE(parse_state, FORMAT_PARSE_FORMAT);
            }
            END_STATE();
        }
        STATE(FORMAT_PARSE_FORMAT) {
            switch (*format_str_ptr) {
                case 'o':
                    input_skip_space();
                    input_to_integer(1, '7');
                    SWITCH_STATE(parse_state, NORMAL_CHAR);
                case 'd':
                    input_skip_space();
                    input_to_integer(0, '9');
                    SWITCH_STATE(parse_state, NORMAL_CHAR);
                case 'u':
                    input_skip_space();
                    input_to_integer(1, '9');
                    SWITCH_STATE(parse_state, NORMAL_CHAR);
                case 'x': case 'X':
                    input_skip_space();
                    input_to_hex(1);
                    SWITCH_STATE(parse_state, NORMAL_CHAR);
                case 'p': case 'P':
                    input_skip_space();
                    input_to_pointer();
                    SWITCH_STATE(parse_state, NORMAL_CHAR);
                case 'c':
                    if (integer_width) format_end_parse();
                    if (max_read_char_num == 0) {
                        max_read_char_num = 1;
                    }
                    input_to_str(0);
                    SWITCH_STATE(parse_state, NORMAL_CHAR);
                case 's':
                    if (integer_width) format_end_parse();
                    input_skip_space();
                    input_to_str(1);
                    SWITCH_STATE(parse_state, NORMAL_CHAR);
                case 'n':
                    if (integer_width || max_read_char_num) format_end_parse();
                    input_put_n();
                    SWITCH_STATE(parse_state, NORMAL_CHAR);
                case '\0':
                default:
                    format_end_parse();
            }
            format_consume_char();
            skip_set_ptr = 0;
            max_read_char_num = 0;
            integer_width = 0;
            END_STATE();
        }
        END_STATE_MACHINE();
    }
}
#endif
