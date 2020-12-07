#include <assert.h>
#include <mm.h>
#include <stddef.h>

/*******************************************************************************
 * @brief 内存页表，跟踪系统的全部内存
 *
 ******************************************************************************/
unsigned char mem_map[PAGING_PAGES] = { 0 };

/*******************************************************************************
 * @brief 从地址 from 拷贝一页数据到 to
 *
 * @param from 源地址
 * @param to 目标地址
 ******************************************************************************/
static inline void
copy_page(uint64_t from, uint64_t to)
{
    for (size_t i = 0; i < PAGE_SIZE / 8; ++i) {
        *(uint64_t*)to = *(uint64_t*)(from);
        to += 8;
        from += 8;
    }
}

/*******************************************************************************
 * @brief 释放指定的物理地址所在的页
 *
 * @param addr 物理地址
 ******************************************************************************/
void
free_page(uint64_t addr)
{
    if (addr < LOW_MEM)
        return;
    if (addr >= HIGH_MEMORY)
        panic("trying to free nonexistent page");
    addr -= LOW_MEM;
    addr >>= 12;
    if (mem_map[addr]--)
        return;
    mem_map[addr] = 0;
    panic("trying to free free page");
}

/*******************************************************************************
 * @brief 获取空物理页
 *
 * @return 物理页的物理地址
 * @note   当没有可分配的物理页时，返回 0
 ******************************************************************************/
uint64_t
get_free_page(void)
{
    int i = PAGING_PAGES - 1;
    for (; i >= LOW_MEM >> 12; --i) {
        if (mem_map[i] == 0) {
            mem_map[i] = 1;
            uint64_t ret = MEM_START + i * PAGE_SIZE;
            memset((void*)ret, 0, PAGE_SIZE);
            return ret;
        }
    }

    if (i < 0) {
        return 0;
    }
}

/*******************************************************************************
 * @brief 测试物理内存分配/回收函数是否正确
 ******************************************************************************/
void
mem_test()
{
    /** 测试 mem_map[] 是否正确 */
    size_t i;
    for (i = 0; i < LOW_MEM >> 12; ++i)
        assert(mem_map[i] == USED);
    for (; i < PAGING_PAGES; ++i)
        assert(mem_map[i] == 0);

    /** 测试物理页分配是否正常 */
    uint64_t page1, old_page1;
    page1 = old_page1 = get_free_page();
    assert((page1 == HIGH_MEMORY - PAGE_SIZE);
    for (size_t i = 0; i < 511; ++i)
    {
        assert(*(uint64_t*)old_page1 == 0);
        old_page1 = old_page1 + 8; 
    }

    uint64_t page2, old_page2;
    page2 = old_page2 = get_free_page();
    assert((page2 == HIGH_MEMORY - 2 * PAGE_SIZE);
    assert(page1 != page2);
    for (size_t i = 0; i < 511; ++i)
    {
        assert(*(uint64_t*)old_page2 == 0);
        old_page2 = old_page2 + 8; 
    }

    uint64_t page3, old_page3;
    page3 = old_page3 =  get_free_page();
    assert((page3 == HIGH_MEMORY - 3 * PAGE_SIZE);
    for (size_t i = 0; i < 511; ++i)
    {
        assert(*(uint64_t*)old_page3 == 0);
        old_page3 = old_page3 + 8; 
    }

    /** 测试返回地址是否不同 */
    assert(page1 != page2);
    assert(page1 != page3);
    assert(page2 != page3);

    free_page(page2);
    assert(page2 == get_free_page());

    /** 测试 mem_map[] 引用计数是否正常 */
    assert (mem_map[(page1 - LOW_MEM) >> 12] == 1);
    assert (mem_map[(page2 - LOW_MEM) >> 12] == 1);
    assert (mem_map[(page3 - LOW_MEM) >> 12] == 1);

    size_t index = (page2 - LOW_MEM) >> 12;
    ++mem_map[index];
    free_page(page2);
    assert (mem_map[index] == 1);
    free_page(page2);
    assert (mem_map[index] == 0);

    /* 通过测试 */
    free_page(page1);
    assert (mem_map[(page1 - LOW_MEM) >> 12] == 1);
    free_page(page3);
    assert (mem_map[(page3 - LOW_MEM) >> 12] == 1);
    kputs("mem_test(): Pass");
}

/*******************************************************************************
 * @brief 初始化内存管理模块
 *
 * 该函数初始化 memmap[] 数组，将物理地址空间 [start_mem, end_mem] 纳入到
 * 内核的管理中。
 *
 * @param start_mem 起始物理地址
 * @param end_mem 结束物理地址
 ******************************************************************************/
void
mem_init(uint64_t start_mem, uint64_t end_mem)
{
    for (size_t i = 0; i < PAGING_PAGES; i++)
        mem_map[i] = USED;
    i = MAP_NR(start_mem);
    end_mem -= start_mem;
    end_mem >>= 12;
    while (end_mem-- > 0)
        mem_map[i++] = 0;
}
