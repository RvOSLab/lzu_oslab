/**
 * 实现物理内存管理
 */
#include <assert.h>
#include <kdebug.h>
#include <mm.h>
#include <stddef.h>

/*******************************************************************************
 * @brief 内存页表，跟踪系统的全部内存
 ******************************************************************************/
unsigned char mem_map[PAGING_PAGES] = { 0 };

/*******************************************************************************
 * @brief 初始化内存管理模块
 *
 * 该函数初始化 mem_map[] 数组，将物理地址空间 [HIGH_MEM, LOW_MEM) 纳入到
 * 内核的管理中。
 ******************************************************************************/
void mem_init()
{
	size_t i;
	for (i = 0; i < PAGING_PAGES; i++)
		mem_map[i] = USED;
	size_t mem_start = LOW_MEM;
	size_t mem_end = HIGH_MEM;
	mem_start = LOW_MEM;
	mem_end = HIGH_MEM;
	i = MAP_NR(mem_start);
	if (mem_end > HIGH_MEM)
		mem_end = HIGH_MEM;
	mem_end -= mem_start;
	mem_end >>= 12;
	while (mem_end-- > 0)
		mem_map[i++] = UNUSED;
	mem_start = MEM_START;
	mem_end = MEM_END;
}

/*******************************************************************************
 * @brief 从地址 from 拷贝一页数据到地址 to
 *
 * @param from 源地址
 * @param to 目标地址
 ******************************************************************************/
static inline void copy_page(uint64_t from, uint64_t to)
{
	for (size_t i = 0; i < PAGE_SIZE / 8; ++i) {
		*(uint64_t *)to = *(uint64_t *)(from);
		to += 8;
		from += 8;
	}
}

/*******************************************************************************
 * @brief 释放指定的物理地址所在的页
 *
 * @param addr 物理地址
 ******************************************************************************/
void free_page(uint64_t addr)
{
	if (addr < LOW_MEM)
		return;
	if (addr >= HIGH_MEM)
		panic("trying to free nonexistent page");
	assert(mem_map[MAP_NR(addr)] != 0, "trying to free free page");
	--mem_map[MAP_NR(addr)];
}

/*******************************************************************************
 * @brief 获取空物理页
 *
 * @return 成功时物理页的物理地址,失败时返回 0
 ******************************************************************************/
uint64_t get_free_page(void)
{
	int i = MAP_NR(HIGH_MEM);
	for (; i >= MAP_NR(LOW_MEM); --i) {
		if (mem_map[i] == 0) {
			mem_map[i] = 1;
			uint64_t ret = MEM_START + i * PAGE_SIZE;
			memset((void *)ret, 0, PAGE_SIZE);
			return ret;
		}
	}
	return 0;
}

/*******************************************************************************
 * @brief 测试物理内存分配/回收函数是否正确
 ******************************************************************************/
void mem_test()
{
	/** 测试 mem_map[] 是否正确 */
	size_t i;
	for (i = 0; i < MAP_NR(LOW_MEM); ++i)
		assert(mem_map[i] == USED, "Error in mem_map[]");
	for (; i < MAP_NR(HIGH_MEM); ++i)
		assert(mem_map[i] == UNUSED, "Error in mem_map[]");

	/** 测试物理页分配是否正常 */
	uint64_t page1, old_page1;
    assert(page1 = old_page1 = get_free_page(), "Memory exhausts");
	assert(page1 != 0, "page1 equal to zero");
	assert(page1 == HIGH_MEM - PAGE_SIZE,
	       "Error in address return by get_free_page()");
	for (i = 0; i < 512; ++i) {
		assert(*(uint64_t *)old_page1 == 0, "page1 is dirty");
		old_page1 = old_page1 + 8;
	}

	uint64_t page2, old_page2;
        assert(page2 = old_page2 = get_free_page(), "Memory exhausts");
	assert(page2 != 0, "page2 equal to zero");
	assert(page2 == HIGH_MEM - 2 * PAGE_SIZE,
	       "page2 is not equal to HIGH_MEM - 2 * PAGE_SIZE");
	assert(page1 != page2, "page1 equal to page2");
	for (i = 0; i < 512; ++i) {
		assert(*(uint64_t *)old_page2 == 0, "page2 is dirty");
		old_page2 = old_page2 + 8;
	}

	uint64_t page3, old_page3;
	assert(page3 = old_page3 = get_free_page(), "Memory exhausts");
	assert(page3 != 0, "page3 equal to zero");
	assert(page3 == HIGH_MEM - 3 * PAGE_SIZE,
	       "page3 is not equal to HIGH_MEM - 3 * PAGE_SIZE");
	for (i = 0; i < 512; ++i) {
		assert(*(uint64_t *)old_page3 == 0, "page3 is dirty");
		old_page3 = old_page3 + 8;
	}

	/** 测试返回地址是否正常 */
	assert(page1 != page2, "page1 equal to page2");
	assert(page1 != page3, "page1 equal to page3");
	assert(page2 != page3, "page2 equal to page3");

	/** 测试 mem_map[] 引用计数是否正常 */
	assert(mem_map[MAP_NR(page2)] == 1, "Reference counter goes wrong");
	free_page(page2);
	assert(mem_map[MAP_NR(page2)] == 0, "Reference counter goes wrong");
	assert(page2 == get_free_page(),
	       "get_free_page() don't return the highest empty page");
	assert(mem_map[MAP_NR(page1)] == 1, "Reference counter goes wrong");
	assert(mem_map[MAP_NR(page2)] == 1, "Reference counter goes wrong");
	assert(mem_map[MAP_NR(page2)] == 1, "Reference counter goes wrong");

	++mem_map[MAP_NR(page2)];
	free_page(page2);
	assert(mem_map[MAP_NR(page2)] == 1, "Reference counter goes wrong");
	free_page(page2);
	assert(mem_map[MAP_NR(page2)] == 0, "Reference counter goes wrong");

	/* 通过测试 */
	free_page(page1);
	assert(mem_map[MAP_NR(page1)] == 0, "Reference counter goes wrong");
	free_page(page3);
	assert(mem_map[MAP_NR(page3)] == 0, "Reference counter goes wrong");
	kputs("mem_test(): Passed");
}
