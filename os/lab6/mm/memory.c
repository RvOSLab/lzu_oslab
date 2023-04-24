#ifdef COMPILE
/**
 * @file memory.c
 * @brief 实现虚拟内存
 *
 * 在阅读代码时要分清物理地址和虚拟地址，否则会导致混乱。
 * 本模块注释中专门写了函数参数是物理地址还是虚拟地址，如果没有写，默认是虚拟地址。
 */
#include <assert.h>
#include <kdebug.h>
#include <mm.h>
#include <stddef.h>

/** 内核页目录（定义在 entry.s 中）*/
extern uint64_t boot_pg_dir[512];

/** 当前进程的页目录 */
uint64_t *pg_dir = boot_pg_dir;

/** 内存页表，跟踪系统的全部内存 */
unsigned char mem_map[PAGING_PAGES] = { 0 };

/**
 * @brief 将物理地址区域映射到虚拟地址区域
 *
 * @param paddr_start 起始物理地址
 * @param paddr_end 结束物理地址
 * @param vaddr 起始虚拟地址
 * @param flag PTE 标志位
 * @note
 *      - 地址必须按页对齐
 *      - 仅建立映射，不修改物理页引用计数
 */
static inline void map_pages(uint64_t paddr_start, uint64_t paddr_end,
                             uint64_t vaddr, uint16_t flag) {
    while (paddr_start < paddr_end) {
        put_page(paddr_start, vaddr, flag);
        paddr_start += PAGE_SIZE;
        vaddr += PAGE_SIZE;
    }
}

/**
 * @brief 建立所有进程共有的内核映射
 *
 * 所有进程发生系统调用、中断、异常后都会进入到内核态，因此所有进程的虚拟地址空间
 * 都要包含内核的部分。
 *
 * 本函数仅创建映射，不会修改 mem_map[] 引用计数
 */
void map_kernel() {
    // map_pages(DEVICE_START, DEVICE_END, DEVICE_ADDRESS, KERN_RW | PAGE_VALID);
    map_pages(MEM_START, MEM_END, KERNEL_ADDRESS, KERN_RWX | PAGE_VALID);
}

/**
 * @brief 激活当前进程页表
 * @note 置位 status 寄存器 SUM 标志位，允许内核读写用户态内存
 */
void active_mapping() {
    __asm__ __volatile__("csrs sstatus, %0\n\t"
                         "csrw satp, %1\n\t"
                         "sfence.vma\n\t"
                         : /* empty output list */
                         : "r"(1 << 18),
                           "r"((PHYSICAL((uint64_t)pg_dir) >> 12) |
                               ((uint64_t)8 << 60)));
}

/**
 * @brief 初始化内存管理模块
 *
 * - 初始化 mem_map[] 数组，将物理地址空间 [MEM_START, HIGH_MEM) 纳入到
 * 内核的管理中。SBI 和内核部分被设置为`USED`，其余内存被设置为`UNUSED`
 * - 初始化页表。
 * - 开启分页
 */
void mem_init() {
    memset(bss_start, 0, kernel_end - bss_start);
    size_t i = MAP_NR(HIGH_MEM);
    /** 设用户内存空间[LOW_MEM, HIGH_MEM)为可用 */
    while (i > MAP_NR(LOW_MEM))
        mem_map[--i] = UNUSED;
    /** 设SBI与内核内存空间[MEM_START, LOW_MEM)的内存空间为不可用 */
    while (i > MAP_NR(MEM_START))
        mem_map[--i] = USED;

    /* 进入 main() 时开启了 RV39
   * 大页模式，暂时创造一个虚拟地址到物理地址的映射让程序跑起来。
   * 现在，我们要新建一个页目录并开启页大小为 4K 的 RV39 分页。*/
    uint64_t page = get_free_page();
    assert(page, "mem_init(): fail to allocate page");
    pg_dir = (uint64_t *)VIRTUAL(page);
    map_kernel();
    active_mapping();
}

/**
 * @brief 从地址 from 拷贝一页数据到地址 to
 *
 * @param from 源地址
 * @param to 目标地址
 */
static inline void copy_page(uint64_t from, uint64_t to) {
    for (size_t i = 0; i < PAGE_SIZE / 8; ++i) {
        *(uint64_t *)to = *(uint64_t *)(from);
        to += 8;
        from += 8;
    }
}

/**
 * @brief 释放指定的物理地址所在的页
 *
 * @param addr 物理地址
 */
void free_page(uint64_t addr) {
    if (addr < LOW_MEM)
        return;
    if (addr >= HIGH_MEM)
        panic("free_page(): trying to free nonexistent page");
    assert(mem_map[MAP_NR(addr)] != 0, "free_page(): trying to free free page");
    --mem_map[MAP_NR(addr)];
}

/**
 * @brief 获取空物理页
 *
 * @return 成功则物理页的物理地址,失败返回 0
 */
uint64_t get_free_page(void) {
    /* fix warrning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    size_t i = MAP_NR(HIGH_MEM) - 1;
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

/**
 * @brief 建立物理地址和虚拟地址间的映射
 *
 * 本函数仅仅建立映射，不修改物理页引用计数
 * 当分配物理页失败（创建页表）时 panic,因此不需要检测返回值。
 *
 * @param page   物理地址
 * @param addr   虚拟地址
 * @param flag   标志位
 * @return 物理地址 page
 * @see panic(), map_kernel()
 * @note 地址需要按页对齐
 */
uint64_t put_page(uint64_t page, uint64_t addr, uint16_t flag) {
    assert((page & (PAGE_SIZE - 1)) == 0,
           "put_page(): Try to put unaligned page %p to %p", page, addr);
    uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr), GET_VPN3(addr) };
    uint64_t *page_table = pg_dir;
    for (size_t level = 0; level < 2; ++level) {
        uint64_t idx = vpns[level];
        if (!(page_table[idx] & PAGE_VALID)) {
            uint64_t tmp;
            assert(tmp = get_free_page(), "put_page(): Memory exhausts");
            page_table[idx] = (tmp >> 2) | PAGE_VALID;
        }
        page_table = (uint64_t *)VIRTUAL(GET_PAGE_ADDR(page_table[idx]));
    }
    page_table[vpns[2]] = (page >> 2) | flag;
    return page;
}

/**
 * 将虚拟地址 addr 所在的虚拟页映射到某物理页
 *
 * @param addr 虚拟地址
 * @param flag 标志位
 * @note
 * 请确保该虚拟地址没有映射到物理页,否则本函数会覆盖原来的映射，导致内存泄漏。
 */
void get_empty_page(uint64_t addr, uint16_t flag) {
    uint64_t tmp;
    assert(tmp = get_free_page(), "get_empty_page(): Memory exhausts");
    put_page(tmp, addr, flag | PAGE_VALID);
}

/**
 * @brief 释放虚拟地址 from 开始的 size 字节
 *
 * 本函数每次释放 N * 2M
 * 地址空间（二级页表一项映射的内存大小），因此虚拟地址`from`需要 对其到 2M
 * 边界，否则 panic。
 *
 * 这个函数被`exit()`调用，用于释放进程虚拟地址空间。
 *
 * @param from 起始地址
 * @param size 要释放的字节数
 * @see exit(), fork()
 */
void free_page_tables(uint64_t from, uint64_t size) {
    assert((from & 0x1FFFFF) == 0,
           "free_page_tables() called with wrong alignment");
    size = (size + 0x1FFFFF) & ~0x1FFFFF;
    int is_user_space = IS_USER(from, from + size);
    assert(
        IS_KERNEL(from, from + size) || IS_USER(from, from + size),
        "free_page_tables(): address space [from, from + size) must be kernel "
        "space or user space");
    size >>= 21;
    uint64_t vpns[3] = { GET_VPN1(from), GET_VPN2(from), GET_VPN3(from) };
    uint64_t dir_idx = vpns[0];
    uint64_t dir_idx_end = dir_idx + ((size * 0x200000) / 0x40000000 - 1) +
                           ((size * 0x200000) % 0x40000000 != 0);
    assert(dir_idx_end < 512, "free_page_tables(): call with wrong argument");
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
            (uint64_t *)VIRTUAL(GET_PAGE_ADDR(pg_dir[dir_idx])) + vpns[1];
        for (; cnt-- > 0; ++pg_tb1) {
            if (!*pg_tb1)
                continue;
            uint64_t *pg_tb2 = (uint64_t *)VIRTUAL(GET_PAGE_ADDR(*pg_tb1));
            /* 用户地址空间：释放页表和指向的物理页 */
            /* 内核地址空间：仅释放页表 */
            if (is_user_space) {
                for (size_t nr = 512; nr-- > 0; pg_tb2++) {
                    if (*pg_tb2) {
                        free_page(GET_PAGE_ADDR(*pg_tb2));
                        *pg_tb2 = 0;
                    }
                }
            }
            free_page(GET_PAGE_ADDR(*pg_tb1));
            *pg_tb1 = 0;
        }
        /* 释放二级页表 */
        if (vpns[1] == 0 &&
            pg_tb1 >
                (uint64_t *)VIRTUAL(GET_PAGE_ADDR(pg_dir[dir_idx])) + 511) {
            free_page(GET_PAGE_ADDR(pg_dir[dir_idx]));
            pg_dir[dir_idx] = 0;
        }
        vpns[1] = 0;
    }
    invalidate();
}

/**
 * @brief 将虚拟地址 from 开始的 size 字节虚拟地址空间拷贝到另一进程虚拟地址 to
 * 处
 *
 * 本函数并没有拷贝内存，而是让两段虚拟地址空间共享同一映射。`size`将被对齐到
 * 2M， 每次拷贝 N * 2M 地址空间（二级页表项映射的内存大小）。
 *
 * 算法：
 *     将页表看成一棵高度为 3 的多叉树。将当前进程虚拟地址 [from, from + size)
 * 拷贝到 目标进程虚拟地址空间 [to, to + size) 就是层次遍历两颗树，将 [from,
 * from + size) 对应的页表项拷贝到 [to, to + size) 对应的页表项中。
 *
 *     实现的关键在于维护当前进程和目标进程虚拟地址（页表项）之间的对应关系。
 *
 * @param from 当前进程虚拟地址
 * @param to_pg_dir 目的进程页目录 **线性映射虚拟地址**
 * @param from 目标进程虚拟地址
 * @param size 要共享的字节数
 * @return 由于滥用`assert()`，导致返回值失效，暂时返回 1
 * @todo  重构，解决滥用`assert()`的问题，当出错时清理资源并返回 0
 * @see free_page_tables(), map_kernel()，copy_mem()
 * @note
 * - `to`开始的 N * 2M 虚拟地址空间必须是 **未映射的**,否则 panic。
 * - 两虚拟地址空间要么都是用户空间，要么都是内核空间
 */
int copy_page_tables(uint64_t from, uint64_t *to_pg_dir, uint64_t to,
                     uint64_t size) {
    assert(!(from & 0x1FFFFF) && !(to & 0x1FFFFF),
           "copy_page_tables() called with wrong alignment");
    size = (size + 0x1FFFFF) & ~0x1FFFFF;
    int is_user_space = IS_USER(from, from + size);
    /* 两虚拟地址空间要么都是用户空间，要么都是内核空间 */
    assert(!(IS_KERNEL(from, from + size) ^ IS_KERNEL(to, to + size)),
           "copy_page_tables(): called with wrong argument");
    size >>= 21;
    uint64_t src_vpns[3] = { GET_VPN1(from), GET_VPN2(from), GET_VPN3(from) };
    uint64_t src_dir_idx = src_vpns[0];
    uint64_t src_dir_idx_end = src_dir_idx +
                               ((size * 0x200000) / 0x40000000 - 1) +
                               ((size * 0x200000) % 0x40000000 != 0);
    uint64_t dest_vpns[3] = { GET_VPN1(to), GET_VPN2(to), GET_VPN3(to) };
    uint64_t dest_dir_idx = dest_vpns[0];
    uint64_t dest_dir_idx_end = dest_dir_idx +
                                ((size * 0x200000) / 0x40000000 - 1) +
                                ((size * 0x200000) % 0x40000000 != 0);
    assert(src_dir_idx_end < 512,
           "copy_page_tables(): called with wrong argument");
    assert(dest_dir_idx_end < 512,
           "copy_page_tables(): called with wrong argument");

    for (; src_dir_idx <= src_dir_idx_end; ++src_dir_idx, ++src_vpns[0]) {
        if (!pg_dir[src_dir_idx]) {
            dest_vpns[1] += 512 - src_vpns[1];
            if (dest_vpns[1] >= 512) {
                dest_vpns[1] %= 512;
                ++dest_vpns[0];
            }
            dest_dir_idx = dest_vpns[0];
            assert(dest_dir_idx < 512, "exceed boundary of to_pg_dir[]");
            continue;
        }
        if (!to_pg_dir[dest_dir_idx]) {
            uint64_t tmp = get_free_page();
            assert(tmp, "copy_page_tables(): memory exhausts");
            to_pg_dir[dest_dir_idx] = (tmp >> 2) | PAGE_VALID;
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
            (uint64_t *)VIRTUAL(GET_PAGE_ADDR(pg_dir[src_dir_idx])) +
            src_vpns[1];
        uint64_t *dest_pg_tb1 =
            (uint64_t *)VIRTUAL(GET_PAGE_ADDR(to_pg_dir[dest_dir_idx])) +
            dest_vpns[1];

        for (; cnt-- > 0; ++src_pg_tb1, ++src_vpns[1]) {
            if (!*src_pg_tb1) {
                ++dest_vpns[1];
                ++dest_pg_tb1;
                if (dest_vpns[1] >= 512) {
                    dest_vpns[1] = 0;
                    ++dest_vpns[0];
                    ++dest_dir_idx;
                    assert(dest_dir_idx < 512,
                           "exceed boundary of to_pg_dir[]");
                    assert(dest_dir_idx == dest_vpns[0],
                           "dest_dir_idx don't sync with dest_vpns[]");
                    dest_pg_tb1 = (uint64_t *)VIRTUAL(
                                      GET_PAGE_ADDR(to_pg_dir[dest_dir_idx])) +
                                  dest_vpns[1];
                }
                continue;
            }
            if (!*dest_pg_tb1) {
                uint64_t tmp = get_free_page();
                assert(tmp, "copy_page_tables(): Memory exhausts");
                *dest_pg_tb1 = (tmp >> 2) | PAGE_VALID;
            } else {
                panic("copy_page_tables(): page table %p already exist",
                      GET_PAGE_ADDR(*dest_pg_tb1));
            }

            uint64_t *src_pg_tb2 =
                (uint64_t *)VIRTUAL(GET_PAGE_ADDR(*src_pg_tb1));
            uint64_t *dest_pg_tb2 =
                (uint64_t *)VIRTUAL(GET_PAGE_ADDR(*dest_pg_tb1));
            for (size_t nr = 512; nr-- > 0; ++src_pg_tb2, ++dest_pg_tb2) {
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
                if (is_user_space) {
                    ++mem_map[MAP_NR(page_addr)];
                    *dest_pg_tb2 &= ~PAGE_WRITABLE;
                    *src_pg_tb2 &= ~PAGE_WRITABLE;
                }
            }
            ++dest_pg_tb1;
            ++dest_vpns[1];
            assert(dest_vpns[0] == dest_dir_idx,
                   "dest_dir_idx don't sync with dest_vpns[0]");
            if (dest_vpns[1] >= 512) {
                dest_vpns[1] = 0;
                ++dest_vpns[0];
                ++dest_dir_idx;
                assert(dest_dir_idx < 512, "exceed boundary of to_pg_dir[]");
                dest_pg_tb1 = (uint64_t *)VIRTUAL(
                                  GET_PAGE_ADDR(to_pg_dir[dest_dir_idx])) +
                              dest_vpns[1];
            }
        }
        src_vpns[1] = 0;
    }
    invalidate();
    return 0;
}

/**
 * @brief 取消页表项对应的页的写保护
 *
 * @param table_entry 页表项指针(虚拟地址)
 */
void un_wp_page(uint64_t *table_entry) {
    uint64_t old_page, new_page;
    old_page = GET_PAGE_ADDR(*table_entry);
    if (old_page >= LOW_MEM && mem_map[MAP_NR(old_page)] == 1) {
        *table_entry |= PAGE_WRITABLE;
        invalidate();
        return;
    }
    assert(new_page = get_free_page(), "un_wp_page(): failed to get free page");
    if (old_page >= LOW_MEM)
        free_page(old_page);
    copy_page(VIRTUAL(old_page), VIRTUAL(new_page));
    *table_entry = (new_page >> 2) | GET_FLAG(*table_entry) | PAGE_WRITABLE;
    invalidate();
}

/**
 * @brief 取消某地址的写保护
 *
 * 地址必须合法，否则 panic
 *
 * @param addr 虚拟地址
 */
void write_verify(uint64_t addr) {
    uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr), GET_VPN3(addr) };
    uint64_t *page_table = pg_dir;
    for (size_t level = 0; level < 2; ++level) {
        uint64_t idx = vpns[level];
        assert(page_table[idx], "write_verify(): addr %p is not available",
               addr);
        page_table = (uint64_t *)VIRTUAL(GET_PAGE_ADDR(page_table[idx]));
    }
    un_wp_page(&page_table[vpns[2]]);
}

/**
 * @brief 测试内存模块是否正常
 */
void mem_test() {
    kputs("mem_test(): running");
    /** 测试虚拟地址到物理地址的线性映射是否正确 */
    uint64_t addr = KERNEL_ADDRESS;
    uint64_t end = KERNEL_ADDRESS + PAGING_MEMORY;
    for (; addr < end; addr += PAGE_SIZE) {
        uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr), GET_VPN3(addr) };
        uint64_t *page_table = pg_dir;
        for (size_t level = 0; level < 2; ++level) {
            uint64_t idx = vpns[level];
            assert(page_table[idx], "page table %p of %p not exists",
                   &page_table[idx], addr);
            page_table = (uint64_t *)VIRTUAL(GET_PAGE_ADDR(page_table[idx]));
        }
        assert(GET_PAGE_ADDR(page_table[vpns[2]]) == PHYSICAL(addr),
               "mem_test(): virtual address %p maps to physical address %p",
               addr, GET_PAGE_ADDR(page_table[vpns[2]]));
    }

    /*
   * [>* 测试虚拟地址[DEVICE_ADDRESS, DEVICE_ADDRESS + DEVICE_END -
   * DEVICE_START)是否映射到了[DEVICE_START, DEVICE_END) <] addr =
   * PLIC_START_ADDR; end = CEIL(UART_END_ADDRESS); for (; addr < end; addr +=
   * PAGE_SIZE) { uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr),
   *                  GET_VPN3(addr) };
   *     uint64_t *page_table = pg_dir;
   *     for (size_t level = 0; level < 2; ++level) {
   *         uint64_t idx = vpns[level];
   *         assert(page_table[idx],
   *                "page table %p of %p not exists",
   *                &page_table[idx], addr);
   *         page_table = (uint64_t *)VIRTUAL(
   *             GET_PAGE_ADDR(page_table[idx]));
   *     }
   *     assert(GET_PAGE_ADDR(page_table[vpns[2]]) ==
   *                addr + UART_START - UART_ADDRESS,
   *            "virtual address %p maps to physical address %p", addr,
   *            GET_PAGE_ADDR(page_table[vpns[2]]));
   * }
   */

    /* 分配 1000 个物理页并映射到虚拟地址 0x200000 开始的 1000 个虚拟页，
   * 这个虚拟地址空间可以看成一个进程的虚拟地址空间 */
    uint64_t page_tracker[1000];
    uint64_t p = 0x200000;
    for (size_t i = 0; i < 1000; ++i) {
        page_tracker[i] = get_free_page();
        assert(page_tracker[i]);
        put_page(page_tracker[i], p, USER_RWX | PAGE_VALID);
        p += PAGE_SIZE;
    }

    /* 检测映射和引用计数是否正确 */
    addr = 0x200000;
    end = addr + 1000 * PAGE_SIZE;
    for (size_t i = 0; addr < end; addr += PAGE_SIZE, ++i) {
        uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr), GET_VPN3(addr) };
        uint64_t *page_table = pg_dir;
        for (size_t level = 0; level < 2; ++level) {
            uint64_t idx = vpns[level];
            assert(page_table[idx], "page table %p of %p not exists",
                   &page_table[idx], addr);
            page_table = (uint64_t *)VIRTUAL(GET_PAGE_ADDR(page_table[idx]));
            assert(mem_map[MAP_NR(PHYSICAL((uint64_t)page_table))] == 1,
                   "page table reference is wrong");
        }
        assert(GET_PAGE_ADDR(page_table[vpns[2]]) == page_tracker[i],
               "virtual address %p maps to physical address %p", addr,
               GET_PAGE_ADDR(page_table[vpns[2]]));
    }

    /* 新建一个页目录（模拟创建新进程），将当前“进程”对应的虚拟地址空间 [0x200000,
   * 0x200000 + 1000*PAGE_SIZE) 拷贝到新“进程”应的虚拟地址空间 [0x200000,
   * 0x200000 + 1000*PAGE_SIZE) */
    uint64_t page = get_free_page();
    assert(page != 0, "failed to allocate memory");
    uint64_t *new_pg_dir = (uint64_t *)VIRTUAL(page);
    uint64_t *old_pg_dir = pg_dir;
    pg_dir = new_pg_dir;
    map_kernel();
    pg_dir = old_pg_dir;
    copy_page_tables(0x200000, new_pg_dir, 0x200000, 1000 * PAGE_SIZE);

    /* 检查旧“进程”虚拟地址空间的映射和引用计数 */
    addr = 0x200000;
    end = addr + 1000 * PAGE_SIZE;
    for (size_t i = 0; addr < end; addr += PAGE_SIZE, ++i) {
        uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr), GET_VPN3(addr) };
        uint64_t *page_table = pg_dir;
        for (size_t level = 0; level < 2; ++level) {
            uint64_t idx = vpns[level];
            assert(page_table[idx], "page table %p of %p not exists",
                   &page_table[idx], addr);
            page_table = (uint64_t *)VIRTUAL(GET_PAGE_ADDR(page_table[idx]));
            assert(mem_map[MAP_NR(PHYSICAL((uint64_t)page_table))] == 1,
                   "page table reference is wrong");
        }
        assert(GET_PAGE_ADDR(page_table[vpns[2]]) == page_tracker[i],
               "virtual address %p maps to physical address %p", addr,
               GET_PAGE_ADDR(page_table[vpns[2]]));
        /* 两个“进程”的虚拟地址同时映射到一个物理地址 */
        assert(mem_map[MAP_NR(page_tracker[i])] == 2,
               "page reference is wrong");
        /* 共享同一物理地址后进行写保护 */
        assert(GET_FLAG(page_table[vpns[2]]) == (USER_RX | PAGE_VALID),
               "permission is wrong");
    }

    /* 检查新“进程”虚拟地址空间的映射和引用计数
   * 新“进程”虚拟地址空间和旧“进程”虚拟地址相同 */
    pg_dir = new_pg_dir;
    addr = 0x200000;
    end = addr + 1000 * PAGE_SIZE;
    for (size_t i = 0; addr < end; addr += PAGE_SIZE, ++i) {
        uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr), GET_VPN3(addr) };
        uint64_t *page_table = pg_dir;
        for (size_t level = 0; level < 2; ++level) {
            uint64_t idx = vpns[level];
            assert(page_table[idx], "page table %p of %p not exists",
                   &page_table[idx], addr);
            page_table = (uint64_t *)VIRTUAL(GET_PAGE_ADDR(page_table[idx]));
            assert(mem_map[MAP_NR(PHYSICAL((uint64_t)page_table))] == 1,
                   "page table reference is wrong");
        }
        assert(GET_PAGE_ADDR(page_table[vpns[2]]) == page_tracker[i],
               "virtual address %p maps to physical address %p", addr,
               GET_PAGE_ADDR(page_table[vpns[2]]));
        assert(mem_map[MAP_NR(GET_PAGE_ADDR(page_table[vpns[2]]))] == 2,
               "page reference is wrong");
        assert(GET_FLAG(page_table[vpns[2]]) == (USER_RX | PAGE_VALID),
               "permission is wrong");
    }

    /* 释放新“进程”虚拟地址空间 */
    pg_dir = new_pg_dir;
    addr = 0x200000;
    end = addr + 1000 * PAGE_SIZE;
    free_page_tables(addr, 1000 * PAGE_SIZE);

    /* 检查旧“进程”虚拟地址空间的映射和引用计数 */
    pg_dir = old_pg_dir;
    for (size_t i = 0; addr < end; addr += PAGE_SIZE, ++i) {
        uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr), GET_VPN3(addr) };
        uint64_t *page_table = pg_dir;
        for (size_t level = 0; level < 2; ++level) {
            uint64_t idx = vpns[level];
            assert(page_table[idx], "page table %p of %p not exists",
                   &page_table[idx], addr);
            page_table = (uint64_t *)VIRTUAL(GET_PAGE_ADDR(page_table[idx]));
            assert(mem_map[MAP_NR(PHYSICAL((uint64_t)page_table))] == 1,
                   "page table reference is wrong");
        }
        assert(GET_PAGE_ADDR(page_table[vpns[2]]) == page_tracker[i],
               "virtual address %p maps to physical address %p", addr,
               GET_PAGE_ADDR(page_table[vpns[2]]));
        /* 新“进程”虚拟地址空间被释放，旧“进程”唯一地占有物理页 */
        assert(mem_map[MAP_NR(GET_PAGE_ADDR(page_table[vpns[2]]))] == 1,
               "page reference is wrong");
        /* 整个过程不涉及旧“进程”虚拟地址空间的写，因此页表项权限不变 */
        assert(GET_FLAG(page_table[vpns[2]]) == (USER_RX | PAGE_VALID),
               "permission is wrong");
    }

    /* 检查新“进程”虚拟地址空间的映射和引用计数 */
    pg_dir = new_pg_dir;
    addr = 0x200000;
    end = addr + 1000 * PAGE_SIZE;
    for (size_t i = 0; addr < end; addr += PAGE_SIZE, ++i) {
        uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr), GET_VPN3(addr) };
        uint64_t *page_table = pg_dir;
        /* 一级页表项被清零
     * 二级页表项清零，但二级页表未被释放
     * 三级页表被释放
     * 所有指向的物理页都被释放 */
        for (size_t level = 0; level < 2; ++level) {
            uint64_t idx = vpns[level];
            if (level > 0) {
                assert(!page_table[idx], "page table %p of %p exists",
                       &page_table[idx], addr);
                break;
            }
            page_table = (uint64_t *)VIRTUAL(GET_PAGE_ADDR(page_table[idx]));
            assert(mem_map[MAP_NR(PHYSICAL((uint64_t)page_table))] == 1,
                   "page table reference is wrong");
        }
    }

    pg_dir = old_pg_dir;
    free_page_tables(0x200000, 1000 * PAGE_SIZE);
    kputs("mem_test(): Passed");
}

/**
 * @brief 打印页表
 */
void show_page_tables() {
    for (size_t i = 0; i++ < 512; ++i) {
        kprintf("%x\n", pg_dir[i]);
        if (pg_dir[i]) {
            uint64_t *pg_tb1 = (uint64_t *)VIRTUAL(GET_PAGE_ADDR(pg_dir[i]));
            for (int j = 512; j-- > 0; ++pg_tb1) {
                kprintf("\t%x\n", *pg_tb1);
                if (*pg_tb1) {
                    uint64_t *pg_tb2 =
                        (uint64_t *)VIRTUAL(GET_PAGE_ADDR(*pg_tb1));
                    for (int k = 512; k-- > 0; ++pg_tb2) {
                        kprintf("\t\t%x\n", *pg_tb2);
                    }
                }
            }
        }
    }
}
#endif
