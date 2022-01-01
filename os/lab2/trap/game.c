#include <uart.h>
#include <kdebug.h>
#include <stddef.h>
// 全局变量
uint64_t position_x, position_y; // 飞机位置
uint64_t bullet_x, bullet_y; // 子弹位置
uint64_t enemy_x, enemy_y; // 敌机位置
uint64_t high, width; //  游戏画面尺寸
uint64_t score; // 得分
uint64_t rand_seed = 10; // 随机数种子

uint64_t rand()
{
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed / 65536) % 32768;
}

void new_enemy() // 新敌机出现
{
    enemy_x = 0;
    enemy_y = rand() % width;
}

void game_start() // 数据初始化
{
    high = 20;
    width = 30;
    position_x = high * 10000 + high / 2;
    position_y = width * 10000 + width / 2;
    bullet_x = -1;
    bullet_y = position_y;
    new_enemy();
    score = 0;
}

void show()
{
    uint64_t i, j;
    for (i = 0; i < high; i++) {
        uart_putc('|');
        for (j = 0; j < width; j++) {
            if ((i == position_x % high) && (j == position_y % width))
                uart_putc('^'); //   输出飞机
            else if ((i == enemy_x) && (j == enemy_y))
                uart_putc('*'); //   输出敌机
            else if ((i == bullet_x) && (j == bullet_y))
                uart_putc('|'); //   输出子弹
            else
                uart_putc(' '); //   输出空白
        }
        uart_putc('|');
        uart_putc('\n');
    }
    kprintf("score: %u\n", score);
}

void detect_hit()
{
    if ((bullet_x == enemy_x) && (bullet_y == enemy_y)) // 子弹击中敌机
    {
        score++; // 分数加1
        new_enemy();
        bullet_x = -1; // 使子弹无效
    }
}
void game_time_update()
{
    if (bullet_x != -1) {
        bullet_x--;
        detect_hit();
    }

    if (enemy_x > high) // 敌机跑出显示屏幕
        new_enemy();
    else {
        enemy_x++;
        detect_hit();
    }
    show();
}

void game_keyboard_update(char input)
{
    switch (input) {
    case 'a':
        position_y--; // 左
        break;
    case 'd':
        position_y++; // 右
        break;
    case 'w':
        position_x--; // 上
        break;
    case 's':
        position_x++; // 下
        break;
    case ' ': // 发射子弹
        bullet_x = (position_x - 1) % high;
        bullet_y = position_y % width;
        break;
    }
    show();
}
