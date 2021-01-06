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
 * @brief 将物理地址 page 映射到虚拟地址 addr
 *
 * 本函数用于初始化页表，不修改映射到的物理地址对应的`mem_map[]`引用计数。
 * @param page 物理地址
 * @param addr 虚拟地址
 * @note 请确保 page 对齐到了 4 K 边界
 ******************************************************************************/
void static map_page(uint64_t page, uint64_t addr)
{
	uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr), GET_VPN3(addr) };
	uint64_t *page_table = pg_dir;
	for (size_t level = 0; level < 2; ++level) {
		uint64_t idx = vpns[level];
		if (!page_table[idx]) {
			uint64_t tmp;
			assert(tmp = get_free_page(),
			       "map_page(): Memory exhausts ");
			page_table[idx] = (tmp >> 2) | 0x11;
		}
		page_table = (uint64_t *)GET_PAGE_ADDR(page_table[idx]);
	}
	page_table[vpns[2]] = (page >> 2) | 0x1F;
}

/*******************************************************************************
 * @brief 初始化内存管理模块
 *
 * - 初始化 mem_map[] 数组，将物理地址空间 [MEM_START, HIGH_MEM) 纳入到
 * 内核的管理中。SBI 和内核部分被设置为`USED`，其余内存被设置为`UNUSED`
 *
 * - 初始化页表。
 ******************************************************************************/
void mem_init()
{
	size_t i;
	for (i = 0; i < PAGING_PAGES; i++)
		mem_map[i] = USED;
	size_t mem_start = LOW_MEM;
	size_t mem_end = HIGH_MEM;
	i = MAP_NR(mem_start);
	if (mem_end > HIGH_MEM)
		mem_end = HIGH_MEM;
	mem_end -= mem_start;
	mem_end >>= 12;
	while (mem_end-- > 0)
		mem_map[i++] = UNUSED;

	mem_start = DEVICE_START;
	mem_end = DEVICE_END;
	for (; mem_start < mem_end; mem_start += PAGE_SIZE)
		map_page(mem_start, mem_start);

	mem_start = MEM_START;
	mem_end = MEM_END;
	for (; mem_start < mem_end; mem_start += PAGE_SIZE)
		map_page(mem_start, mem_start);
	mem_test();
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
		panic("free_page(): trying to free nonexistent page");
	assert(mem_map[MAP_NR(addr)] != 0,
	       "free_page(): trying to free free page");
	--mem_map[MAP_NR(addr)];
}

/*******************************************************************************
 * @brief 获取空物理页
 *
 * @return 成功则物理页的物理地址,失败返回 0 
 ******************************************************************************/
uint64_t get_free_page(void)
{
	int i = MAP_NR(HIGH_MEM) - 1;
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
 * `put_page()`用于建立物理地址和虚拟地址间的映射，物理页必须是 **已分配但未映射** 的，
 * 不能将未分配或已经映射到别的虚拟页的物理页映射到另一个虚拟页。
 *
 * 当分配物理页失败（创建页表）时 panic,因此不需要检测返回值。
 *
 * @param page 物理地址
 * @param addr 虚拟地址
 * @return 物理地址 page
 * @see put_dirty_page(), panic()
 ******************************************************************************/
uint64_t put_page(uint64_t page, uint64_t addr)
{
	assert((page & (PAGE_SIZE - 1)) == 0,
	       "put_page(): Try to put unaligned page %p to %p", page, addr);
	assert(page >= LOW_MEM && page < HIGH_MEM,
	       "put_page(): Trying to put page %p at %p\n", page, addr);
	assert(mem_map[MAP_NR(page)] == 1,
	       "put_page(): mem_map[] disagrees with %p at %p \n", page, addr);
	uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr), GET_VPN3(addr) };
	uint64_t *page_table = pg_dir;
	for (size_t level = 0; level < 2; ++level) {
		uint64_t idx = vpns[level];
		if (!(page_table[idx] & PAGE_PRESENT)) {
			uint64_t tmp;
			assert(tmp = get_free_page(),
			       "put_page(): Memory exhausts");
			page_table[idx] = (tmp >> 2) | 0x11;
		}
		page_table = (uint64_t *)GET_PAGE_ADDR(page_table[idx]);
	}
	page_table[vpns[2]] = (page >> 2) | 0x1F;
	return page;
}

/*******************************************************************************
 * @brief 将物理地址 page 映射到虚拟地址 addr,并对应页设置为 dirty
 *
 * @param page 物理地址
 * @param addr 虚拟地址
 * @return 物理地址 page
 * @note 当分配物理页失败直接 panic，因此不需要检测返回值
 * @see put_page(), panic()
 ******************************************************************************/
uint64_t put_dirty_page(uint64_t page, uint64_t addr)
{
	put_page(page, addr);
	uint64_t tmp = pg_dir[GET_VPN1(addr)];
	tmp = GET_PAGE_ADDR(tmp);
	tmp = *((uint64_t *)tmp + GET_VPN2(addr));
	tmp = GET_PAGE_ADDR(tmp);
	*((uint64_t *)tmp + GET_VPN3(addr)) |= PAGE_DIRTY;
	return page;
}

/*******************************************************************************
 * 将虚拟地址 addr 所在的虚拟页映射到某物理页
 *
 * @param addr 虚拟地址
 * @note 请确保该虚拟地址没有映射到物理页,否则本函数会覆盖原来的映射，导致内存泄漏。
 ******************************************************************************/
void get_empty_page(uint64_t addr)
{
	uint64_t tmp;
	assert(tmp = get_free_page(), "get_empty_page(): Memory exhausts");
	put_page(tmp, addr);
}

/*******************************************************************************
 * @brief 释放线性地址 from 开始的 size 字节
 *
 * 本函数每次释放 N * 2M 地址空间（二级页表项映射的内存大小），因此线性地址`from`需要
 * 对其到 2M 边界，否则 panic。
 *
 * 这个函数被`exit()`调用，用于释放进程虚拟地址空间。
 *
 * @param from 起始地址
 * @param size 要释放的字节数
 * @see exit(), fork()
 ******************************************************************************/
void free_page_tables(uint64_t from, uint64_t size)
{
	assert((from & 0x1FFFFF) == 0,
	       "free_page_tables() called with wrong alignment");
	size = (size + 0x1FFFFF) >> 21;
	uint64_t vpns[3] = { GET_VPN1(from), GET_VPN2(from), GET_VPN3(from) };
	uint64_t dir_idx = vpns[0];
	uint64_t dir_idx_end = dir_idx + ((size * 0x200000) / 0x40000000 - 1) +
			       ((size * 0x200000) % 0x3FFFFFFF != 0);
	assert(dir_idx_end < 512,
	       "free_page_tables(): call with wrong argument");
	for (; dir_idx <= dir_idx_end; ++dir_idx) {
		if (!pg_dir[dir_idx])
			continue;
		uint64_t cnt;
		if (size >= 512 - vpns[1]) {
			cnt = 512 - vpns[1];
			size -= cnt;
		} else {
			cnt = size;
			size = 0;
		}

		uint64_t *pg_tb1 =
			(uint64_t *)GET_PAGE_ADDR(pg_dir[dir_idx]) + vpns[1];
		for (; cnt-- > 0; ++pg_tb1) {
			if (!*pg_tb1)
				continue;
			uint64_t *pg_tb2 = (uint64_t *)GET_PAGE_ADDR(*pg_tb1);
			for (size_t nr = 512; nr-- > 0; pg_tb2++) {
				if (*pg_tb2) {
					free_page(GET_PAGE_ADDR(*pg_tb2));
					*pg_tb2 = 0;
				}
			}
			free_page(GET_PAGE_ADDR(*pg_tb1));
			*pg_tb1 = 0;
		}
		if (vpns[1] == 0 &&
		    pg_tb1 > (uint64_t *)GET_PAGE_ADDR(pg_dir[dir_idx]) + 511) {
			free_page(GET_PAGE_ADDR(pg_dir[dir_idx]));
			pg_dir[dir_idx] = 0;
		}
		vpns[1] = 0;
	}
	invalidate();
}

/*******************************************************************************
 * @brief 将线性地址 from 开始的 size 字节线性地址空间拷贝到线性地址 to
 *
 * 本函数并没有拷贝内存，而是让两段线性地址空间共享同一映射。`size`将被对其到 2M，
 * 每次拷贝 N * 2M 地址空间（二级页表项映射的内存大小）。
 *
 * 线性地址`from`需要对其到 2M 边界，否则 panic。
 * 两线性地址空间不能重叠，否则 panic。
 * `to`开始的 N * 2M 线性地址空间必须是 **未映射的**,否则 panic。
 *
 * @param from 源
 * @param from 目的地
 * @param size 要共享的字节数
 * @return 由于没有实现交换，因此总是成功，返回 0
 * @see free_page_tables()
 ******************************************************************************/
int copy_page_tables(uint64_t from, uint64_t to, uint64_t size)
{
	assert(!(from & 0x1FFFFF) && !(to & 0x1FFFFF),
	       "copy_page_tables() called with wrong alignment");
	assert(size <= from - to, "copy_page_tables() called with wrong size");
	size = (size + 0x1FFFFF) >> 21;
	uint64_t src_vpns[3] = { GET_VPN1(from), GET_VPN2(from),
				 GET_VPN3(from) };
	uint64_t src_dir_idx = src_vpns[0];
	uint64_t src_dir_idx_end = src_dir_idx +
				   ((size * 0x200000) / 0x40000000 - 1) +
				   ((size * 0x200000) % 0x3FFFFFFF != 0);
	uint64_t dest_vpns[3] = { GET_VPN1(to), GET_VPN2(to), GET_VPN3(to) };
	uint64_t dest_dir_idx = dest_vpns[0];
	assert(dest_dir_idx < 512,
	       "copy_page_tables(): called with wrong argument");
	for (; src_dir_idx <= src_dir_idx_end; ++src_dir_idx, ++dest_dir_idx) {
		assert(dest_dir_idx < 512,
		       "copy_page_tables(): called with wrong argument");
		if (!pg_dir[src_dir_idx]) {
			continue;
		}
		if (!pg_dir[dest_dir_idx]) {
			uint64_t tmp = get_free_page();
			assert(tmp, "copy_page_tables(): memory exhausts");
			pg_dir[dest_dir_idx] = (tmp >> 2) | 0x11;
		}

		uint64_t cnt;
		if (size > 512 - src_vpns[1]) {
			cnt = 512 - src_vpns[1];
			size -= cnt;
		} else {
			cnt = size;
			size = 0;
		}
		uint64_t *src_pg_tb1 =
			(uint64_t *)GET_PAGE_ADDR(pg_dir[src_dir_idx]) +
			src_vpns[1];
		uint64_t *dest_pg_tb1 =
			(uint64_t *)GET_PAGE_ADDR(pg_dir[dest_dir_idx]) +
			dest_vpns[1];

		for (; cnt-- > 0; ++src_pg_tb1, ++dest_pg_tb1) {
			if (!*src_pg_tb1) {
				continue;
			}
			if (!*dest_pg_tb1) {
				uint64_t tmp = get_free_page();
				assert(tmp,
				       "copy_page_tables(): Memory exhausts");
				*dest_pg_tb1 = (tmp >> 2) | 0x11;
			} else {
				panic("copy_page_tables(): already exist 1");
			}

			uint64_t *src_pg_tb2 =
				(uint64_t *)GET_PAGE_ADDR(*src_pg_tb1);
			uint64_t *dest_pg_tb2 =
				(uint64_t *)GET_PAGE_ADDR(*dest_pg_tb1);
			for (size_t nr = 512; nr-- > 0;
			     ++src_pg_tb2, ++dest_pg_tb2) {
				if (!*src_pg_tb2) {
					continue;
				}
				if (*dest_pg_tb2) {
					panic("copy_page_tables(): already exist 2");
				}
				*dest_pg_tb2 = *src_pg_tb2;
				*dest_pg_tb2 &= ~PAGE_READABLE;
				uint64_t page_addr = GET_PAGE_ADDR(*src_pg_tb2);
				if (page_addr >= LOW_MEM) {
					++mem_map[MAP_NR(page_addr)];
				}
			}
		}
		src_vpns[1] = dest_vpns[1] = 0;
	}
	return 0;
}

/*******************************************************************************
 * @brief 取消页表项对应的页的写保护
 *
 * @param table_entry 页表项指针（物理地址）
 ******************************************************************************/
void un_wp_page(uint64_t *table_entry)
{
	uint64_t old_page, new_page;
	old_page = GET_PAGE_ADDR(*table_entry);
	if (old_page >= LOW_MEM && mem_map[MAP_NR(old_page)] == 1) {
		*table_entry |= PAGE_WRITABLE;
		invalidate();
		return;
	}
	assert(new_page = get_free_page(),
	       "un_wp_page(): failed to get free page");
	if (old_page >= LOW_MEM)
		mem_map[MAP_NR(old_page)]--;
	copy_page(old_page, new_page);
	*table_entry = (new_page >> 2) | 0x1F;
	invalidate();
}

/*******************************************************************************
 * @brief 取消某地址的写保护
 *
 * @param addr 线性地址
 ******************************************************************************/
void write_verify(uint64_t addr)
{
	uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr), GET_VPN3(addr) };
	uint64_t *page_table = pg_dir;
	for (size_t level = 0; level < 2; ++level) {
		uint64_t idx = vpns[level];
		if (!page_table[idx]) {
			return;
		}
		page_table = (uint64_t *)GET_PAGE_ADDR(page_table[idx]);
	}
	un_wp_page(&page_table[vpns[2]]);
}
/*******************************************************************************
 * @brief 缺页异常处理函数
 *
 * @param error_code x86 异常错误码
 * @param addr 异常发生地址
 * @todo 改写为 RISCV
 ******************************************************************************/
void do_no_page(uint64_t error_code, uint64_t addr)
{
}

/*******************************************************************************
 * @brief 写保护异常处理函数
 *
 * @param error_code x86 错误码
 * @param addr 异常发生地址
 * @todo 改写为 RISCV
 ******************************************************************************/
void do_wp_page(uint64_t error_code, uint64_t addr)
{
}

/*******************************************************************************
 * @brief 测试页表是否正确初始化
 ******************************************************************************/
void mem_test()
{
	kputs("mem_test(): running");
	/* 测试虚拟地址`MEM_START`至`MEM_END`是否映射到了相等的物理地址 */
	uint64_t addr = MEM_START;
	for (; addr < MEM_END; addr += PAGE_SIZE) {
		uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr),
				     GET_VPN3(addr) };
		uint64_t *page_table = pg_dir;
		for (size_t level = 0; level < 2; ++level) {
			uint64_t idx = vpns[level];
			assert(page_table[idx],
			       "page table %p of %p not exists",
			       &page_table[idx], addr);
			page_table = (uint64_t *)GET_PAGE_ADDR(page_table[idx]);
		}
		assert(GET_PAGE_ADDR(page_table[vpns[2]]) == addr,
		       "mem_test(): linear address %p maps to physical address %p",
		       addr, GET_PAGE_ADDR(page_table[vpns[2]]));
	}
	/* 测试虚拟地址`DEVICE_START`至`DEVICE_END`是否映射到了相等的物理地址 */
	addr = DEVICE_START;
	for (; addr < DEVICE_END; addr += PAGE_SIZE) {
		uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr),
				     GET_VPN3(addr) };
		uint64_t *page_table = pg_dir;
		for (size_t level = 0; level < 2; ++level) {
			uint64_t idx = vpns[level];
			assert(page_table[idx],
			       "page table %p of %p not exists",
			       &page_table[idx], addr);
			page_table = (uint64_t *)GET_PAGE_ADDR(page_table[idx]);
		}
		assert(GET_PAGE_ADDR(page_table[vpns[2]]) == addr,
		       "linear address %p maps to physical address %p", addr,
		       GET_PAGE_ADDR(page_table[vpns[2]]));
		addr += PAGE_SIZE;
	}
	/* 测试 copy_page_tables() 是否正确 */
	copy_page_tables(MEM_START, MEM_END, MEM_END - MEM_START);
	uint64_t mem_end = MEM_END + MEM_END - MEM_START;
	addr = MEM_END;

	/* 线性地址 [MEM_END, MEM_END + MEM_END - MEM_START) 映射到物理地址 [MEM_START, MEM_END) */
	for (; addr < mem_end; addr += PAGE_SIZE) {
		uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr),
				     GET_VPN3(addr) };
		uint64_t *page_table = pg_dir;
		for (size_t level = 0; level < 2; ++level) {
			uint64_t idx = vpns[level];
			assert(page_table[idx],
			       "page table %p of %p not exists",
			       &page_table[idx], addr);
			page_table = (uint64_t *)GET_PAGE_ADDR(page_table[idx]);
		}
		assert(GET_PAGE_ADDR(page_table[vpns[2]]) ==
			       addr - MEM_END + MEM_START,
		       "virtual address %p maps to physical address %p", addr,
		       GET_PAGE_ADDR(page_table[vpns[2]]));
	}

	/* 测试引用计数是否正确
     *
     * copy_page_tables() 会递增引用计数，所有主内存区的页引用计数都会递增
     * 因此，主内存区中的引用计数一定大于等于 1。
     *
     * 源线性地址空间 [mem_start, mem_end) 对应的页表最初引用计数为 1, 
     * copy_page_tables() 后变成 2
     */
	for (size_t idx = MAP_NR(HIGH_MEM) - 1; idx > MAP_NR(LOW_MEM); --idx)
		assert(mem_map[idx] >= 1);

	addr = MEM_START;
	for (; addr < MEM_END; addr += PAGE_SIZE) {
		uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr),
				     GET_VPN3(addr) };
		uint64_t *page_table = pg_dir;
		for (size_t level = 0; level < 2; ++level) {
			uint64_t idx = vpns[level];
			assert(page_table[idx],
			       "page table %p of %p not exists",
			       &page_table[idx], addr);
			page_table = (uint64_t *)GET_PAGE_ADDR(page_table[idx]);
			assert(mem_map[MAP_NR((uint64_t)page_table)] == 2,
			       "mem_test(): mem_map[] references are errorneous");
		}
		assert(GET_PAGE_ADDR(page_table[vpns[2]]) == addr,
		       "linear address %p maps to physical address %p", addr,
		       GET_PAGE_ADDR(page_table[vpns[2]]));
	}

	/* 测试 free_page_tables() */
	free_page_tables(MEM_END, MEM_END - MEM_START);
	for (size_t idx = MAP_NR(HIGH_MEM) - 1; idx > MAP_NR(LOW_MEM); --idx)
		assert(mem_map[idx] < 2,
		       "mem_test(): mem_map[] references are errorneous");

	addr = MEM_START;
	for (; addr < MEM_END; addr += PAGE_SIZE) {
		uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr),
				     GET_VPN3(addr) };
		uint64_t *page_table = pg_dir;
		for (size_t level = 0; level < 2; ++level) {
			uint64_t idx = vpns[level];
			assert(page_table[idx],
			       "page table %p of %p not exists",
			       &page_table[idx], addr);
			page_table = (uint64_t *)GET_PAGE_ADDR(page_table[idx]);
			/* 线性地址 [MEM_END, MEM_END + MEM_END - MEM_START)被释放，
             * 因此主内存区 [MEM_START, MEM_END) 中页表的引用计数为 1*/
			assert(mem_map[MAP_NR((uint64_t)page_table)] == 1,
			       "mem_test(): mem_map[] references are errorneous");
		}
		assert(GET_PAGE_ADDR(page_table[vpns[2]]) == addr,
		       "linear address %p maps to physical address %p", addr,
		       GET_PAGE_ADDR(page_table[vpns[2]]));
	}

	/* [MEM_END, MEM_END + MEM_END - MEM_START) 对应的所有二三级页表项均为空 */
	addr = MEM_END;
	mem_end = MEM_END + MEM_END - MEM_START;
	for (; addr < mem_end; addr += PAGE_SIZE) {
		uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr),
				     GET_VPN3(addr) };
		uint64_t *page_table = pg_dir;
		for (size_t level = 0; level < 2; ++level) {
			uint64_t idx = vpns[level];
			if (level > 0) {
				assert(!page_table[idx],
				       "mem_test(): pte %p is not empty",
				       &page_table[idx]);
			}
			page_table = (uint64_t *)GET_PAGE_ADDR(page_table[idx]);
		}
	}
	kputs("mem_test(): Passed");
}
