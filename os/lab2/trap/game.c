#include <uart.h>
#include <kdebug.h>
#include <stddef.h>
// 全局变量
#define HIGH 20 // 游戏画面高
#define WIDTH 30 // 游戏画面宽
#define BULLET_MAX 10 // 最多子弹数
#define ENEMY_MAX 10 // 最多敌人数
char screen[HIGH][WIDTH + 4]; // 游戏画面
uint64_t position_x, position_y; // 飞机位置
struct bullet {
    uint64_t x;
    uint64_t y;
} bullets[BULLET_MAX]; // 子弹
uint64_t bullet_index = 0; // 子弹数量
struct enemy {
    uint64_t x;
    uint64_t y;
} enemies[ENEMY_MAX]; // 敌机
uint64_t enemy_num = 1; // 敌机数量
uint64_t score; // 得分
uint64_t rand_seed = 10; // 随机数种子

uint64_t rand()
{
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed / 65536) % 32768;
}

void new_enemy(uint64_t i) // 新敌机出现
{
    enemies[i].x = 0;
    enemies[i].y = rand() % WIDTH;
}

void game_start() // 数据初始化
{
    position_x = HIGH * 10000 + HIGH / 2;
    position_y = WIDTH * 10000 + WIDTH / 2;
    new_enemy(0);
    score = 0;
}

void show()
{
    uint64_t i, j;
    for (i = 0; i < HIGH; i++) {
        screen[i][0] = '|';
        for (j = 1; j < WIDTH; screen[i][j++] = ' ')
            ;
        screen[i][j++] = '|';
        screen[i][j++] = '\n';
        screen[i][j] = '\r';
    }
    screen[position_x % HIGH][position_y % WIDTH + 1] = '^';
    for (i = 0; i < enemy_num; i++)
        screen[enemies[i].x][enemies[i].y + 1] = '*';
    for (i = 0; i < BULLET_MAX; i++)
        if (bullets[i].x != -1)
            screen[bullets[i].x][bullets[i].y + 1] = '|';

    for (i = 0; i < HIGH * (WIDTH + 4); i++) {
        uart_putc(*((char *)screen + i));
    }
    kprintf("score: %u\n", score);
}

void detect_hit()
{
    for (uint64_t i = 0; i < BULLET_MAX; i++) {
        for (uint64_t j = 0; j < enemy_num; j++) {
            if (((bullets[i].x == enemies[j].x) ||
                 (bullets[i].x - 1 == enemies[j].x)) &&
                bullets[i].y == enemies[j].y) // 子弹击中敌机
            {
                score++; // 分数加1
                new_enemy(j);
                bullets[i].x = -1; // 使子弹无效
            }
        }
    }
}
void game_time_update()
{
    detect_hit();
    for (int bullet_i = 0; bullet_i < BULLET_MAX; bullet_i++)
        if (bullets[bullet_i].x != -1)
            bullets[bullet_i].x--;
    for (int enemy_i = 0; enemy_i < enemy_num; enemy_i++)
        if (enemies[enemy_i].x > HIGH)
            new_enemy(enemy_i);
        else
            enemies[enemy_i].x++;
    show();
}

void shoot()
{
    bullets[bullet_index].x = (position_x - 1) % HIGH;
    bullets[bullet_index].y = position_y % WIDTH;
    bullet_index = (bullet_index + 1) % BULLET_MAX;
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
        shoot();
        break;
    }
    show();
}
