#include <uart.h>
#include <kdebug.h>
#include <stddef.h>
#include <rtc.h>

#define CLEAR_LINE "\033[2K" // 控制序列：清除整行
// 全局变量
#define HIGH 20 // 游戏画面高
#define WIDTH 30 // 游戏画面宽
#define BULLET_MAX 10 // 最多子弹数
#define ENEMY_MAX 10 // 最多敌人数
#define ENEMY_GROW_SPEED 3 // 敌人增加速度(每N分数增加一个敌人)
void show();

uint64_t position_x, position_y; // 飞机位置
struct bullet {
    uint64_t x;
    uint64_t y;
} bullets[BULLET_MAX]; // 子弹
uint64_t bullet_index; // 下一子弹下标
struct enemy {
    uint64_t x;
    uint64_t y;
} enemies[ENEMY_MAX]; // 敌机
uint64_t die; // 是否死亡
uint64_t enemy_num; // 敌机数量
uint64_t score; // 得分
uint64_t missing, missing_max; // 漏掉的敌机数量/最大可漏掉的敌机数量
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
    enemy_num = 1;
    bullet_index = 0;
    score = 0;
    die = 0;
    missing = 0;
    missing_max = 1;
    for (int i = 0; i < BULLET_MAX; i++) {
        bullets[i].x = -1;
    }

    new_enemy(0);
    show();
    set_alarm(read_time() + 1000000000);
}

void show()
{
    char screen[HIGH][WIDTH + 4]; // 游戏画面
    uint64_t i, j;
    kprintf(CLEAR_LINE "score: %u, missing: %u/%u\n", score, missing, missing_max);

    for (i = 0; i < HIGH; i++) {
        screen[i][0] = '|';
        for (j = 1; j < WIDTH + 1; screen[i][j++] = ' ')
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
    kprintf("\033[%uA", HIGH + 1); // 回到顶部
}

void detect_hit()
{
    for (uint64_t i = 0; i < BULLET_MAX; i++) {
        for (uint64_t j = 0; j < enemy_num; j++) {
            if (((bullets[i].x == enemies[j].x) || (bullets[i].x - 1 == enemies[j].x)) && bullets[i].y == enemies[j].y) // 子弹击中敌机
            {
                score++; // 分数加1
                if (score % ENEMY_GROW_SPEED == 0) {
                    missing_max++; // 奖励最大漏掉的敌机数量加 1
                    if (score < ENEMY_MAX * ENEMY_GROW_SPEED) { // 每N分数出现一个敌机
                        enemy_num++; // 敌机数量加1
                        new_enemy(enemy_num - 1);
                    }
                }
                new_enemy(j);
                bullets[i].x = -1; // 使子弹无效
            }
        }
    }
}

void do_die(const char *message)
{
    clear_alarm();
    die = 1;
    kprintf(message);
}

void detect_be_hit()
{
    for (uint64_t i = 0; i < enemy_num; i++) {
        if (enemies[i].x == position_x % HIGH && enemies[i].y == position_y % WIDTH) {
            do_die(CLEAR_LINE "YOU ARE HIT, PRESS r TO RESTART.\r");
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
        if (enemies[enemy_i].x >= HIGH - 1) { // 敌机将超出边界
            missing++;
            if (missing > missing_max) {
                do_die(CLEAR_LINE "MISSING TO MANY ENEMIES, PRESS r TO RESTART.\r");
                return;
            } else
                new_enemy(enemy_i);
        } else
            enemies[enemy_i].x++;
    show();
    detect_be_hit();
}

void shoot()
{
    bullets[bullet_index].x = (position_x - 1) % HIGH;
    bullets[bullet_index].y = position_y % WIDTH;
    bullet_index = (bullet_index + 1) % BULLET_MAX;
}

void game_keyboard_update(char input)
{
    if (die) {
        if (input == 'r') {
            game_start();
        }
    } else {
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
        detect_be_hit();
    }
}
