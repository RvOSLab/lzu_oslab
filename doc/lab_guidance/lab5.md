这一节中，我们将在上一节的基础上完成虚拟页共享、写时复制等虚拟内存机制。

实际的系统中虚拟地址和物理地址之间并不是线性的映射关系，一个虚拟地址可能被映射到任何可用的物理地址，上一节实现的虚拟地址-物理地址线性映射仅仅是一个过渡。虚拟地址到物理地址的映射关系是系统运行时由内核动态创建的。

# 写时复制

系统中往往有多个进程共存，每个进程（除了进程 0）都有父进程，子进程会继承父进程的代码、数据，但又有自己不同的代码、数据。这类似于自然界的遗传现象，子女会继承父母的基因，但和父母又有所不同。

```c
#include <stdio.h>
#include <unistd.h>
int value = 100;
int
main()
{
    pid_t pid = fork();
    if (pid < 0) { // fork() 失败
        fprintf(stderr, "fork() failed\n");
    } else if (pid == 0) { // 父进程
        value = 200;
        printf("parent's value: %d\n", value);
    } else { // 子进程
        printf("child's value: %d\n", value);
    }
    return 0;
}
// 可能的输出
// child's value: 100
// parent's value: 200

```

以上代码片段创建一个子进程，并分别在父子进程中打印`value`的值。在父进程中，`value`的值是`100`，`fork()`出子进程后，子进程也拥有一份`value`的副本，两个进程的值互不干扰。在这个简单的例子中，父子进程共享代码并拥有不同的`value`（`value`的不同副本），在更复杂的多进程环境下，多个进程可能拥有更多的变量。

实现这种”继承“的最简单办法是直接拷贝。以上面的程序为例，在`fork()`子进程时，可以把父进程的全部代码、数据拷贝一份给子进程。父子进程的代码一定是相同的，只有数据可能会被修改，直接拷贝将进行大量不必要的拷贝。

解决直接拷贝的低效的方法是写时复制。写时复制共享代码、数据，将拷贝延迟到进程修改数据时。

```
              ┌─────────────────┐                                     ┌─────────────────┐
              │                 │                                     │                 │
              │                 │                                     │                 │
              │                 │                                     │                 │
              │                 │                                     │                 │◄─────────Parent Process
              │                 │                                     │                 │
              │                 │                                     │                 │
              │                 │                                     │                 │
              │    Code/Data    │◄─────────Parent Process             │    Code/Data    │
              │                 │                                     │                 │
              │                 │                                     │                 │
              │                 │                                     │                 │
              │                 │                                     │                 │◄─────────Child Process
              │                 │                                     │                 │
              │                 │                                     │                 │
              │                 │                                     │                 │
              └─────────────────┘                                     └─────────────────┘



──────────────────────────────────────────────────────┬─────────────────────────────────────────────────────────────────
                                                      │
                                                      │
                                                      │
                                                      │
                                                      │  Child modify data
                                                      │
                                                      │
                                                      │
                                                      │
                                                      ▼



               ┌─────────────────┐                Copy                 ┌─────────────────┐
               │                 │                                     │                 │
               │                 │    ─────────────────────────────►   │                 │
               │                 │                                     │                 │
               │                 │                                     │                 │
               │                 │                                     │                 │
               │                 │                                     │                 │
               │                 │                                     │                 │
               │    Code/Data    │◄─────────Parent Process             │    Code/Data    │◄─────────Child Process
               │                 │                                     │                 │
               │                 │                                     │                 │
               │                 │                                     │                 │
               │                 │                                     │                 │
               │                 │                                     │                 │
               │                 │                                     │                 │
               │                 │                                     │                 │
               └─────────────────┘                                     └─────────────────┘
```

在上面的图示中，父进程创建子进程后，两个进程共享同一块代码、数据，不存在拷贝。子进程修改数据时，内核将这块数据拷贝一份给子进程，这时父子进程不再共享这块数据，子进程在它自己的副本上修改。

为了实现写时复制，内核必须发现并拦截进程的写操作，在进程写之前拷贝数据。回忆前面的页表结构，页表项中有标志位 W 控制物理页是否可写，程序试图写只读的物理页将产生异常（写保护）。不同进程共享同一物理页时，我们清空标志位 W，禁止进程写该页;进程写该页时，处理器产生异常，内核将发现并捕获异常，从而拦截进程的写操作。内核捕获异常后，设置标志位 W 并拷贝数据。



# 共享（拷贝）虚拟页

了解写时复制的原理后，现在来实现虚拟页的共享。共享虚拟页其实就是让两个进程的虚拟页映射到同一块物理页。

```
VP: Virtual Page
PP: Physical Page

  VP1              VP1    VP2
   │                │      │
   │                │      │
   ▼                ▼      ▼
┌──────┐   Share    ┌──────┐
│  PP  │  ────────► │  PP  │
└──────┘            └──────┘
```

完成共享只需要设置虚拟页到物理页的映射关系。由于是两个进程共享一个物理页，需要同步修改两个进程的页表，而且两个页表都是三级页表，这给实现带来一定难度。

系统仅在创建进程时共享虚拟页，而创建进程不太可能仅共享几页虚拟页，因此我们将共享的最小虚拟页数量设置为 512 个（2 M），这个数量恰好是一个页表所能够容纳的页表项数。也就是说，每次至少共享一个页表管理的虚拟内存区域。

两个进程页表的修改必须保持同步，比如将进程 A 的 [100, 500) 拷贝到进程 B 的 [400, 800)，进程 A 的​ 100 对应进程 B 的 400，进程 A 的 200 对应进程 B 的 500。将多级页表看成一棵高度为 3 的多叉树，节点为页表，将当前进程虚拟地址 [from, from + size) 拷贝到目标进程虚拟地址空间 [to, to + size) 就是层次遍历两颗树，将 [from, from + size)对应的页表项拷贝到 [to, to + size) 对应的页表项中并进行写保护。

遍历的过程中存在边界条件，比如上诉进程 A 地址 [100, 500) 在一个页表中，而进程 B 地址 [400, 800) 在两个页表中。我们要考虑到遍历过程中地址区域在不同页表中的情况。具体实现使用一主一从的方式，源进程的页表树为主，目的进程的页表树为辅，在遍历源页表树的过程中访问目的进程页表树。

为了避免权限问题，两地址区间要么都是用户空间，要么都是内核空间。

内核态的地址空间是内核，所有进程共享，不需要拷贝，所以不需要写保护。

```c
int copy_page_tables(uint64_t from, uint64_t *to_pg_dir, uint64_t to,
		     uint64_t size)
{
	assert(!(from & 0x1FFFFF) && !(to & 0x1FFFFF),
	       "copy_page_tables() called with wrong alignment");
	size = (size + 0x1FFFFF) & ~0x1FFFFF;
	int is_user_space = IS_USER(from, from + size);
	/* 两虚拟地址空间要么都是用户空间，要么都是内核空间 */
	assert(!(IS_KERNEL(from, from + size) ^ IS_KERNEL(to, to + size)),
	       "copy_page_tables(): called with wrong argument");
	size >>= 21;
	uint64_t src_vpns[3] = { GET_VPN1(from), GET_VPN2(from),
				 GET_VPN3(from) };
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
			to_pg_dir[dest_dir_idx] = (tmp >> 2) | 0x01;
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
						to_pg_dir[dest_dir_idx])) +
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
		            dest_pg_tb1 = (uint64_t *)VIRTUAL(GET_PAGE_ADDR(
						    to_pg_dir[dest_dir_idx])) +
					    dest_vpns[1];
                }
				continue;
			}
			if (!*dest_pg_tb1) {
				uint64_t tmp = get_free_page();
				assert(tmp,
				       "copy_page_tables(): Memory exhausts");
				*dest_pg_tb1 = (tmp >> 2) | 0x01;
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
                dest_pg_tb1 = (uint64_t *)VIRTUAL(GET_PAGE_ADDR(
						to_pg_dir[dest_dir_idx])) +
					dest_vpns[1];
            }
		}
		src_vpns[1] = 0;
	}
	invalidate();
	return 0;
}
```

# 取消写保护

在拷贝虚拟地址空间时对虚拟页进行写保护，程序尝试写只读虚拟页会产生异常，内核将捕获异常并处理。因此，取消写保护的操作要作为异常处理函数。

RISC-V 的支持的异常如下：

![RISC-V interupt and exception](images/interupt-and-exception.png)

显然，写只读内存页对应的异常是 *Store page fault*，取消写保护应作为 *Store page fault* 的异常处理例程。

取消写保护时，置位发生异常的地址对应的页表项 W 标志，拷贝物理页。有一种特殊情况，虽然虚拟页标记为只读，但该页没有被共享，这时只需要置位 W 标志即可。

```c
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
```

发生 *Store page fault* 时，发生异常的物理地址将被保存在 stval 寄存器中，我们通过物理地址定位页表项。

```c
void write_verify(uint64_t addr)
{
	uint64_t vpns[3] = { GET_VPN1(addr), GET_VPN2(addr), GET_VPN3(addr) };
	uint64_t *page_table = pg_dir;
	for (size_t level = 0; level < 2; ++level) {
		uint64_t idx = vpns[level];
		assert (page_table[idx],
                "write_verify(): addr %p is not available", addr);
		page_table =
			(uint64_t *)VIRTUAL(GET_PAGE_ADDR(page_table[idx]));
	}
	un_wp_page(&page_table[vpns[2]]);
}
```

至此，我们完成了实现虚拟内存的保护和共享机制所需的函数，在后面的实验中将完成进程并将`write_verify`放入到异常处理的代码中。

# 问题

1. 系统在何时创建虚拟地址到物理地址的映射？

- 创建进程
- 缺页
- 写保护异常



2. 何时会出现虚拟页只读，但未共享的情况？

进程 A 创建进程 B，进程 B 结束，进程 A 部分被共享的虚拟页是只读的。

