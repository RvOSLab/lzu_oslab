这一节我们将完成物理内存的管理。

#  物理内存的布局

通常，我们将内存看成是一个字节数组，地址是数组的下标。但实际上这个模型只能算是内存的抽象，真实的物理地址空间不一定从 0 开始寻址，也不一定是连续的。

物理内存地址空间是内存、内存映射 IO 和一些只读存储器的集合，其中的地址不一定都指向物理内存，地址空间中还存在许多空洞。这里介绍 OS 实验基于的硬件平台（qemu模拟器）的物理地址空间布局：

```c
// https://github.com/qemu/qemu/blob/master/hw/riscv/virt.c
static const struct MemmapEntry {
    hwaddr base;
    hwaddr size;
} virt_memmap[] = {
    [VIRT_DEBUG] =       {        0x0,         0x100 },
    [VIRT_MROM] =        {     0x1000,        0xf000 },
    [VIRT_TEST] =        {   0x100000,        0x1000 },
    [VIRT_RTC] =         {   0x101000,        0x1000 },
    [VIRT_CLINT] =       {  0x2000000,       0x10000 },
    [VIRT_PCIE_PIO] =    {  0x3000000,       0x10000 },
    [VIRT_PLIC] =        {  0xc000000, VIRT_PLIC_SIZE(VIRT_CPUS_MAX * 2) },
    [VIRT_UART0] =       { 0x10000000,         0x100 },
    [VIRT_VIRTIO] =      { 0x10001000,        0x1000 },
    [VIRT_FLASH] =       { 0x20000000,     0x4000000 },
    [VIRT_PCIE_ECAM] =   { 0x30000000,    0x10000000 },
    [VIRT_PCIE_MMIO] =   { 0x40000000,    0x40000000 },
    [VIRT_DRAM] =        { 0x80000000,           0x0 },
};
```

本实验中，RAM 地址从`0x8000_0000`到`0x8800_0000`，共 128M。其中`0x8000_0000`到`0x8020_0000`存放 SBI，`0x8020_0000`开始存放内核，内核之后的地址空间由用户进程使用。

物理地址空间中的空洞没有任何意义，不能使用。除 RAM 外，物理地址空间中还有大量内存映射 IO，我们会在后续的实验中介绍。这里通过查看 qemu 源代码获取物理地址空间布局，实际上 RISCV 使用设备树（*device tree*）协议来描述硬件信息，并在启动后扫描硬件并将结果以 DTB（*Device Tree Blob*)格式写入到物理内存中，将内存起始地址传入 a1 寄存器。内核可以解析 DTB 来获取硬件信息，避免硬编码。设备树协议将在后续实验中介绍。

```c
// include/mm.h: 定义物理内存空间相关的宏
#define PAGE_SIZE 4096
#define FLOOR(addr) ((addr) / PAGE_SIZE * PAGE_SIZE)
#define CEIL(addr)                                                             \
	(((addr) / PAGE_SIZE + ((addr) % PAGE_SIZE != 0)) * PAGE_SIZE)
#define DEVICE_START    0x10000000                  /**< 设备树地址空间，暂时不使用 */
#define DEVICE_END      0x10010000
#define MEM_START       0x80000000                  /**< 物理内存地址空间 */
#define MEM_END         0x88000000
#define SBI_START       0x80000000                  /**< SBI 物理内存起始地址 */
#define SBI_END         0x80200000                  /**< 用户程序（包括内核）可用的物理内存地址空间开始 */
#define HIGH_MEM        0x88000000                  /**< 空闲内存区结束 */
#define LOW_MEM         0x82000000                  /**< 空闲内存区开始（可用于用户进程和数据放置） */
#define PAGING_MEMORY   (1024 * 1024 * 128)         /**< 系统物理内存大小 (bytes) */
#define PAGING_PAGES    (PAGING_MEMORY >> 12)       /**< 系统物理内存页数 */
```

为了实现的简单，直接给内核分配了一段足够大的内存 [SBI_END, LOW_MEM)。注意，实验中的地址空间都是左闭右开的区间。

由于本内核仅运行在 qemu 模拟器上，而qemu 模拟器提供的内存布局和大小是固定的，本实验跳过检测物理内存的过程，直接管理物理内存。

# 物理页的管理

我们将物理内存管理起来是为了实现虚拟内存，虚拟内存以页为单位，因此我们也以页为单位管理物理内存。将物理地址空间按页划分，得到许多物理页，每个物理页都有自己的编号，这样物理地址空间就可以看成是一个以物理页为元素的数组，数组下标就是物理页的物理页号。

本实验使用位图（*bitmap*）管理 128M 物理内存，这也是 Linux 内核最初使用的物理内存管理方式。以数组`mem_map[]`表示 128M 物理内存，下标是物理页号，元素代表该物理页的引用计数。

```c
// include/mm.h
#define USED 100
#define UNUSED 0

extern unsigned char mem_map[PAGING_PAGES];
```

物理内存地址从`0x8000_0000`开始，显然，物理地址右移 12 位得到的物理页号不能直接用作`mem_map[]`的下标，需要把物理地址减去`0x8000_0000`再右移 12 位。

```c
// include/mm.h
#define MAP_NR(addr)    (((addr)-MEM_START) >> 12)
```

每当虚拟内存请求物理页，内核就从位图`mem_map[]`查找到空闲物理页并分配。物理地址`0x8000_0000`到`0x8200_0000`被 SBI 占据，其后一段内存被内核占据。这部分物理内存一定不能被分配，否则会导致内核和 SBI 被用户进程覆盖，因此`mem_map[]`中这部分内存被标记为`USED`。

实验中的地址都已经刻意对齐到了 4K 边界，因此不需要上下对齐。

```c
// mm/memory.c
void mem_init()
{
	size_t i = MAP_NR(HIGH_MEM);
	/* 设用户内存空间[LOW_MEM, HIGH_MEM)为可用 */
	while (i > MAP_NR(LOW_MEM))
		mem_map[--i] = UNUSED;
	/* 设SBI与内核内存空间[MEM_START,LOW_MEM)的内存空间为不可用 */
	while (i > MAP_NR(MEM_START))
		mem_map[--i] = USED;
}
```

# 物理页的分配

物理页的分配就是在所有物理页中找到一个空闲（未分配）物理页。实验使用最简单的数据结构管理物理页，自然也使用最简单的方式分配物理页：遍历物理内存位图，查找空闲页并分配。

```c
// mm/memory.c
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
```

分配出来的物理页很可能要作为页表等关键的数据结构使用，因此在分配的过程中就将物理页清零。

# 物理页的释放

物理页的释放就是将某一物理页标记为空闲的（可分配的），以便下次分配。

```c
// mm/memory.c
void free_page(uint64_t addr)
{
	if (addr < LOW_MEM)
		return;
	if (addr >= HIGH_MEM)
		panic("trying to free nonexistent page");
	assert(mem_map[MAP_NR(addr)] != 0, "trying to free free page");
	--mem_map[MAP_NR(addr)];
}
```

  我们会在后面的实验中实现虚拟内存，多个虚拟页可以通过某种方式共享（映射到）一个物理页，物理页的引用计数很可能会大于 1。每次“释放”物理页只需要递减引用计数，当引用计数为 0 时，物理页自然就空闲了。

# 测试

物理内存的分配回收是后面所有模块的基础，一旦物理内存模块出错却没有被发现，在实现后面的虚拟内存、进程、文件系统时就会“写时一时爽，调试火葬场”。

为了验证模块的可靠性，我们会编写一个简单的测试函数`mem_test()`，检查是否正确初始化`mem_map[]`，是否正确分配回收物理页。

测试通过后可以看到`mem_test()`打印：

```
mem_test(): Passed
```



# 总结

我们通过软件实现了物理内存的管理，营造了”物理内存可以增减、分配、释放“的假象。当需要物理内存时，通过`get_free_page()`获取一页;不需要时通过`free_page()`释放物理页。如果软件使用了全部的物理内存，虽然系统有 128M 物理内存，但对于软件来说，物理内存已经耗尽，可用的物理内存为 0 字节。

