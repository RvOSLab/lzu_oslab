/**
 * 实现物理内存管理
 * @todo
 * - `copy_page_tables()`
 * - 区分物理地址和虚拟地址
 * - mem_test()
 */
#include <assert.h>
#include <kdebug.h>
#include <mm.h>
#include <stddef.h>

/*******************************************************************************
 * brief 内核页目录（定义在 entry.s 中）
 ******************************************************************************/
extern uint64_t boot_pg_dir[512];

/*******************************************************************************
 * @brief 当前进程的页目录
 ******************************************************************************/
uint64_t *pg_dir = boot_pg_dir;

/*******************************************************************************
 * @brief 内存页表，跟踪系统的全部内存
 ******************************************************************************/
uint8_t mem_map[PAGING_PAGES] = { 0 };

/*******************************************************************************
 * @brief 将物理地址 page 映射到虚拟地址 addr
 *
 * 本函数仅仅创建虚拟地址和物理地址之间的映射，不修改物理页应用计数，不获取物理页。
 * 这个函数仅用于建立内核映射。
 *
 * @param page 物理地址
 * @param addr 虚拟地址
 * @note 请确保 page 对齐到了 4 K 边界
 ******************************************************************************/
inline void map_page(uint64_t page, uint64_t addr)
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
		page_table =
			(uint64_t *)VIRTUAL(GET_PAGE_ADDR(page_table[idx]));
	}
	page_table[vpns[2]] = (page >> 2) | KERN_RWX;
}

/*******************************************************************************
 * @brief 建立所有进程共有的内核映射
 * 
 * 所有进程发生系统调用、中断、异常后都会进入到内核态，因此所有进程的虚拟地址空间
 * 都要包含内核的部分。
 *
 * 本函数仅创建映射，不会修改 mem_map[] 引用计数
 ******************************************************************************/
void map_kernel()
{
	uint64_t phy_mem_start;
	uint64_t phy_mem_end;
	uint64_t vir_mem_start;

	phy_mem_start = DEVICE_START;
	phy_mem_end = DEVICE_END;
	vir_mem_start = DEVICE_ADDRESS;
	while (phy_mem_start < phy_mem_end) {
		map_page(phy_mem_start, vir_mem_start);
		phy_mem_start += PAGE_SIZE;
		vir_mem_start += PAGE_SIZE;
	}

	phy_mem_start = MEM_START;
	phy_mem_end = MEM_END;
	vir_mem_start = BASE_ADDRESS;
	while (phy_mem_start < phy_mem_end) {
		map_page(phy_mem_start, vir_mem_start);
		phy_mem_start += PAGE_SIZE;
		vir_mem_start += PAGE_SIZE;
	}
}

/*******************************************************************************
 * @brief 激活当前进程页表
 ******************************************************************************/
void active_mapping()
{
	__asm__ __volatile__("csrrs x0, sstatus, %0\n\t"
			     "csrrw x0, satp, %1\n\t"
			     "sfence.vma\n\t"
			     : /* empty output list */
			     : "r"(1 << 18),
			       "r"((PHYSICAL((uint64_t)pg_dir) >> 12) |
				   ((uint64_t)8 << 60)));
}

/*******************************************************************************
 * @brief 初始化内存管理模块
 *
 * - 初始化 mem_map[] 数组，将物理地址空间 [MEM_START, HIGH_MEM) 纳入到
 * 内核的管理中。SBI 和内核部分被设置为`USED`，其余内存被设置为`UNUSED`
 * - 初始化页表。
 * - 开启分页
 ******************************************************************************/
void mem_init()
{
	/* 初始化 mem_map[] */
	size_t i;
	for (i = 0; i < PAGING_PAGES; i++)
		mem_map[i] = USED;
	uint64_t mem_start = LOW_MEM;
	uint64_t mem_end = HIGH_MEM;
	i = MAP_NR(mem_start);
	if (mem_end > HIGH_MEM)
		mem_end = HIGH_MEM;
	mem_end -= mem_start;
	mem_end >>= 12;
	while (mem_end-- > 0)
		mem_map[i++] = UNUSED;

	map_kernel();
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
			memset((void *)VIRTUAL(ret), 0, PAGE_SIZE);
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
 * @param pg_dir 页目录
 * @param page   物理地址
 * @param addr   虚拟地址
 * @param flag   标志位
 * @return 物理地址 page
 * @see panic(), map_page()
 ******************************************************************************/
uint64_t put_page(uint64_t page, uint64_t addr, uint8_t flag)
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
		page_table =
			(uint64_t *)VIRTUAL(GET_PAGE_ADDR(page_table[idx]));
	}
	page_table[vpns[2]] = (page >> 2) | flag;
	return page;
}

/*******************************************************************************
 * 将虚拟地址 addr 所在的虚拟页映射到某物理页
 * 
 * @param addr 虚拟地址
 * @param flag 标志位
 * @note 请确保该虚拟地址没有映射到物理页,否则本函数会覆盖原来的映射，导致内存泄漏。
 ******************************************************************************/
void get_empty_page(uint64_t addr, uint8_t flag)
{
	uint64_t tmp;
	assert(tmp = get_free_page(), "get_empty_page(): Memory exhausts");
	put_page(tmp, addr, flag);
}

/*******************************************************************************
 * @brief 释放虚拟地址 from 开始的 size 字节
 *
 * 本函数每次释放 N * 2M 地址空间（二级页表一项映射的内存大小），因此虚拟地址`from`需要
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
			(uint64_t *)VIRTUAL(GET_PAGE_ADDR(pg_dir[dir_idx])) +
			vpns[1];
		for (; cnt-- > 0; ++pg_tb1) {
			if (!*pg_tb1)
				continue;
			uint64_t *pg_tb2 =
				(uint64_t *)VIRTUAL(GET_PAGE_ADDR(*pg_tb1));
			for (size_t nr = 512; nr-- > 0; pg_tb2++) {
				if (*pg_tb2) {
					free_page(GET_PAGE_ADDR(*pg_tb2));
					*pg_tb2 = 0;
				}
			}
			free_page(GET_PAGE_ADDR(*pg_tb1));
			*pg_tb1 = 0;
		}
		/* 释放二级页表 */
		if (vpns[1] == 0 && pg_tb1 > (uint64_t *)VIRTUAL(GET_PAGE_ADDR(
						     pg_dir[dir_idx])) +
						     511) {
			free_page(GET_PAGE_ADDR(pg_dir[dir_idx]));
			pg_dir[dir_idx] = 0;
		}
		vpns[1] = 0;
	}
	invalidate();
}

/*******************************************************************************
 * @brief 将虚拟地址 from 开始的 size 字节虚拟地址空间拷贝到另一进程虚拟地址 to 处
 *
 * 这个函数用于拷贝用户虚拟地址空间，如果要拷贝内核线性映射，应该调用`map_kernel()`
 *
 * 本函数并没有拷贝内存，而是让两段虚拟地址空间共享同一映射。`size`将被对其到 2M，
 * 每次拷贝 N * 2M 地址空间（二级页表项映射的内存大小）。
 *
 * `to`开始的 N * 2M 虚拟地址空间必须是 **未映射的**,否则 panic。
 *
 * @param from 源
 * @param to_pg_dir 目的进程页目录 **线性映射虚拟地址**
 * @param from 目的地
 * @param size 要共享的字节数
 * @return 由于没有实现交换，因此总是成功，返回 0
 * @todo  重构接口，将`to_pg_dir`改为进程控制块指针
 * @see free_page_tables(), map_kernel()
 * @note 这个函数是本实验最复杂的函数，涉及两个进程间虚拟地址空间的共享
 ******************************************************************************/
int copy_page_tables(uint64_t from, uint64_t *to_pg_dir, uint64_t to,
		     uint64_t size)
{
	assert(from < BASE_ADDRESS && to < BASE_ADDRESS,
	       "try to copy kernel mapping");
	assert(!(from & 0x1FFFFF) && !(to & 0x1FFFFF),
	       "copy_page_tables() called with wrong alignment");
	size = (size + 0x1FFFFF) >> 21;
	uint64_t src_vpns[3] = { GET_VPN1(from), GET_VPN2(from),
				 GET_VPN3(from) };
	uint64_t src_dir_idx = src_vpns[0];
	uint64_t src_dir_idx_end = src_dir_idx +
				   ((size * 0x200000) / 0x40000000 - 1) +
				   ((size * 0x200000) % 0x3FFFFFFF != 0);
	uint64_t dest_vpns[3] = { GET_VPN1(to), GET_VPN2(to), GET_VPN3(to) };
	uint64_t dest_dir_idx = dest_vpns[0];
	uint64_t dest_dir_idx_end = dest_dir_idx +
				    ((size * 0x200000) / 0x40000000 - 1) +
				    ((size * 0x200000) % 0x3FFFFFFF != 0);
	assert(src_dir_idx < 512 && src_dir_idx_end < 512,
	       "copy_page_tables(): called with wrong argument");
	assert(dest_dir_idx < 512 && dest_dir_idx_end < 512,
	       "copy_page_tables(): called with wrong argument");

	for (; src_dir_idx <= src_dir_idx_end; ++src_dir_idx) {
		if (!pg_dir[src_dir_idx]) {
			continue;
		}
		if (!to_pg_dir[dest_dir_idx]) {
			uint64_t tmp = get_free_page();
			assert(tmp, "copy_page_tables(): memory exhausts");
			to_pg_dir[dest_dir_idx] = (tmp >> 2) | 0x11;
		}

		uint64_t cnt;
		if (size > 512 - src_vpns[1]) {
			cnt = 512 - src_vpns[1];
			size -= cnt;
		} else {
			cnt = size;
			size = 0;
		}
		uint64_t *src_pg_tb1 = (uint64_t *)VIRTUAL(GET_PAGE_ADDR(
					       pg_dir[src_dir_idx])) +
				       src_vpns[1];
		uint64_t *dest_pg_tb1 = (uint64_t *)VIRTUAL(GET_PAGE_ADDR(
						pg_dir[dest_dir_idx])) +
					dest_vpns[1];

		// dest_pg_tb1 的值
		for (; cnt-- > 0; ++src_pg_tb1) {
			if (!*src_pg_tb1) {
				continue;
			}
			if (!*dest_pg_tb1) {
				uint64_t tmp = get_free_page();
				assert(tmp,
				       "copy_page_tables(): Memory exhausts");
				*dest_pg_tb1 = (tmp >> 2) | 0x11;
			} else {
				panic("copy_page_tables(): page table %p already exist",
				      GET_PAGE_ADDR(*dest_pg_tb1));
			}

			uint64_t *src_pg_tb2 =
				(uint64_t *)VIRTUAL(GET_PAGE_ADDR(*src_pg_tb1));
			uint64_t *dest_pg_tb2 = (uint64_t *)VIRTUAL(
				GET_PAGE_ADDR(*dest_pg_tb1));
			for (size_t nr = 512; nr-- > 0;
			     ++src_pg_tb2, ++dest_pg_tb2) {
				if (!*src_pg_tb2) {
					continue;
				}
				if (*dest_pg_tb2) {
					panic("copy_page_tables(): PTE is mapped to %p",
					      GET_PAGE_ADDR(*dest_pg_tb2));
				}
				/* 写保护 */
				*dest_pg_tb2 = *src_pg_tb2;
				uint64_t page_addr = GET_PAGE_ADDR(*src_pg_tb2);
				if (page_addr >= LOW_MEM) {
					++mem_map[MAP_NR(page_addr)];
					*dest_pg_tb2 &= ~PAGE_READABLE;
					*src_pg_tb2 &= ~PAGE_READABLE;
				} else {
					/*
                     * 物理地址 [MEM_START, LOW_MEM) 是 OpenSBI 和内核，这段内存被标记为`USED`，不会被分配。
                     * 正常情况下，要拷贝的线性地址不会指向这段内存。
                     */
					panic("copy_page_tables(): try to copy memory below LOW_MEM");
				}
			}
			++dest_pg_tb1;
			++dest_vpns[1];
			assert(dest_vpns[1] < 512,
			       "copy_page_tables(): dest_vpns[] error");
			if (dest_pg_tb1 > (uint64_t *)VIRTUAL(GET_PAGE_ADDR(
						  to_pg_dir[dest_vpns[0]])) +
						  511) {
				if (!to_pg_dir[++dest_dir_idx]) {
					uint64_t tmp = get_free_page();
					assert(tmp,
					       "copy_page_tables(): memory exhausts");
					to_pg_dir[dest_dir_idx] =
						(tmp >> 2) | 0x11;
				}
				dest_pg_tb1 = (uint64_t *)VIRTUAL(
					GET_PAGE_ADDR(to_pg_dir[dest_dir_idx]));
				dest_vpns[1] = 0;
			}
		}
		src_vpns[1] = 0;
	}
	invalidate();
	return 0;
}

/*******************************************************************************
 * @brief 取消页表项对应的页的写保护
 *
 * @param table_entry 页表项指针(虚拟地址)
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
		free_page(old_page);
	copy_page(VIRTUAL(old_page), VIRTUAL(new_page));
	*table_entry = (new_page >> 2) | GET_FLAG(*table_entry) | PAGE_WRITABLE;
	invalidate();
}

/*******************************************************************************
 * @brief 取消某地址的写保护
 * 地址必须合法，否则 panic
 * @param addr 虚拟地址
 ******************************************************************************/
void write_verify(uint64_t addr)
{
	uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr), GET_VPN3(addr) };
	uint64_t *page_table = pg_dir;
	for (size_t level = 0; level < 2; ++level) {
		uint64_t idx = vpns[level];
		if (!page_table[idx]) {
			panic("write_verify(): addr %p is not available", addr);
		}
		page_table =
			(uint64_t *)VIRTUAL(GET_PAGE_ADDR(page_table[idx]));
	}
	un_wp_page(&page_table[vpns[2]]);
}

/******************************************************************************
 *@brief 写保护异常处理函数
 ******************************************************************************/
/* void wp_page_handler(struct trapframe *frame) */
/* {                                             */
/*     uint64_t addr = frame->stval;             */
/*     write_verify(addr);                       */
/* }                                             */

/*******************************************************************************
 * @brief 测试内存模块是否正常
 ******************************************************************************/
void mem_test()
{
	kputs("mem_test(): running");
	/** 测试虚拟地址到物理地址的线性映射是否正确 */
	uint64_t addr = BASE_ADDRESS;
	uint64_t end = BASE_ADDRESS + PAGING_MEMORY;
	for (; addr < end; addr += PAGE_SIZE) {
		uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr),
				     GET_VPN3(addr) };
		uint64_t *page_table = pg_dir;
		for (size_t level = 0; level < 2; ++level) {
			uint64_t idx = vpns[level];
			assert(page_table[idx],
			       "page table %p of %p not exists",
			       &page_table[idx], addr);
			page_table = (uint64_t *)VIRTUAL(
				GET_PAGE_ADDR(page_table[idx]));
		}
		assert(GET_PAGE_ADDR(page_table[vpns[2]]) == PHYSICAL(addr),
		       "mem_test(): virtual address %p maps to physical address %p",
		       addr, GET_PAGE_ADDR(page_table[vpns[2]]));
	}

	/** 测试虚拟地址[DEVICE_ADDRESS, DEVICE_ADDRESS + DEVICE_END - DEVICE_START)是否映射到了[DEVICE_START, DEVICE_END) */
	addr = DEVICE_ADDRESS;
	end = DEVICE_ADDRESS + DEVICE_END - DEVICE_START;
	for (; addr < end; addr += PAGE_SIZE) {
		uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr),
				     GET_VPN3(addr) };
		uint64_t *page_table = pg_dir;
		for (size_t level = 0; level < 2; ++level) {
			uint64_t idx = vpns[level];
			assert(page_table[idx],
			       "page table %p of %p not exists",
			       &page_table[idx], addr);
			page_table = (uint64_t *)VIRTUAL(
				GET_PAGE_ADDR(page_table[idx]));
		}
		assert(GET_PAGE_ADDR(page_table[vpns[2]]) ==
			       addr + DEVICE_START - DEVICE_ADDRESS,
		       "virtual address %p maps to physical address %p", addr,
		       GET_PAGE_ADDR(page_table[vpns[2]]));
	}

	/** 测试 copy_page_tables() 是否正确 */
	addr = BASE_ADDRESS + PAGING_MEMORY;
	end = addr + PAGING_MEMORY;
	copy_page_tables(BASE_ADDRESS, pg_dir, addr, PAGING_MEMORY);

	/* 虚拟地址 [BASE_ADDRESS + PAGING_MEMORY, BASE_ADDRESS + 2*PAGING_MEMORY) 映射到物理地址 [MEM_START, MEM_END) */
	for (; addr < end; addr += PAGE_SIZE) {
		uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr),
				     GET_VPN3(addr) };
		uint64_t *page_table = pg_dir;
		for (size_t level = 0; level < 2; ++level) {
			uint64_t idx = vpns[level];
			assert(page_table[idx],
			       "page table %p of %p not exists",
			       &page_table[idx], addr);
			page_table = (uint64_t *)VIRTUAL(
				GET_PAGE_ADDR(page_table[idx]));
		}
		assert(GET_PAGE_ADDR(page_table[vpns[2]]) ==
			       PHYSICAL(addr - PAGING_MEMORY),
		       "virtual address %p maps to physical address %p", addr,
		       GET_PAGE_ADDR(page_table[vpns[2]]));
	}

	/* copy_page_tables() 会递增主内存区物理页引用计数，因此，主内存区中的引用计数一定大于等于 1。
     *
     * 页表的物理页最初引用计数为 1, copy_page_tables() 后被另一个进程映射，引用计数变为 2
     */
	for (size_t idx = MAP_NR(HIGH_MEM) - 1; idx > MAP_NR(LOW_MEM); --idx)
		assert(mem_map[idx] >= 1 && mem_map[idx] < 3);

	addr = BASE_ADDRESS;
	end = BASE_ADDRESS + PAGING_MEMORY;
	for (; addr < end; addr += PAGE_SIZE) {
		uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr),
				     GET_VPN3(addr) };
		uint64_t *page_table = pg_dir;
		for (size_t level = 0; level < 2; ++level) {
			uint64_t idx = vpns[level];
			assert(page_table[idx],
			       "page table %p of %p not exists",
			       &page_table[idx], addr);
			page_table = (uint64_t *)VIRTUAL(
				GET_PAGE_ADDR(page_table[idx]));
			/* 二、三级页表的引用计数一定是 2 */
			assert(mem_map[MAP_NR(PHYSICAL((uint64_t)page_table))] ==
				       2,
			       "mem_test(): mem_map[] references are errorneous");
		}
		assert(GET_PAGE_ADDR(page_table[vpns[2]]) == PHYSICAL(addr),
		       "virtual address %p maps to physical address %p", addr,
		       GET_PAGE_ADDR(page_table[vpns[2]]));
	}

	/** 测试 free_page_tables() */
	free_page_tables(BASE_ADDRESS + PAGING_MEMORY, PAGING_MEMORY);
	for (size_t idx = MAP_NR(HIGH_MEM) - 1; idx > MAP_NR(LOW_MEM); --idx)
		assert(mem_map[idx] < 2,
		       "mem_test(): mem_map[] references are errorneous");

	/* 测试 [BASE_ADDRESS, BASE_ADDRESS + PAGING_MEMORY) 页表是否正确 */
	addr = BASE_ADDRESS;
	end = BASE_ADDRESS + PAGING_MEMORY;
	for (; addr < end; addr += PAGE_SIZE) {
		uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr),
				     GET_VPN3(addr) };
		uint64_t *page_table = pg_dir;
		for (size_t level = 0; level < 2; ++level) {
			uint64_t idx = vpns[level];
			assert(page_table[idx],
			       "page table %p of %p not exists",
			       &page_table[idx], addr);
			page_table = (uint64_t *)VIRTUAL(
				GET_PAGE_ADDR(page_table[idx]));
			/* 虚拟地址 [BASE_ADDRESS + PAGING_MEMORY, BASE_ADDRESS + 2*PAGING_MEMORY)被释放，
             * 因此主内存区 [MEM_START, MEM_END) 中页表的引用计数为 1*/
			assert(mem_map[MAP_NR(PHYSICAL((uint64_t)page_table))] ==
				       1,
			       "mem_test(): mem_map[] references are errorneous");
		}
		assert(GET_PAGE_ADDR(page_table[vpns[2]]) == PHYSICAL(addr),
		       "virtual address %p maps to physical address %p", addr,
		       GET_PAGE_ADDR(page_table[vpns[2]]));
	}

	/* [BASE_ADDRESS + PAGING_MEMORY, BASE_ADDRESS + 2*PAGING_MEMORY) 对应的所有二三级页表项均为空 */
	addr = BASE_ADDRESS;
	end = BASE_ADDRESS + 2 * PAGING_MEMORY;
	for (; addr < end; addr += PAGE_SIZE) {
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
			page_table = (uint64_t *)VIRTUAL(
				GET_PAGE_ADDR(page_table[idx]));
		}
	}
	kputs("mem_test(): Passed");
}

/*******************************************************************************
 * @brief 打印页表
 ******************************************************************************/
void show_page_tables()
{
	for (int i = 512; i-- > 0; ++i) {
		kprintf("%x\n", pg_dir[i]);
		if (pg_dir[i]) {
			uint64_t *pg_tb1 =
				(uint64_t *)VIRTUAL(GET_PAGE_ADDR(pg_dir[i]));
			for (int j = 512; j-- > 0; ++pg_tb1) {
				kprintf("\t%x\n", *pg_tb1);
				if (*pg_tb1) {
					uint64_t *pg_tb2 = (uint64_t *)VIRTUAL(
						GET_PAGE_ADDR(*pg_tb1));
					for (int k = 512; k-- > 0; ++pg_tb2) {
						kprintf("\t\t%x\n", *pg_tb2);
					}
				}
			}
		}
	}
}
