#include <mm.h>
#include <kdebug.h>
#include <swap.h>
#include <sched.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

static uint64_t clock_algrithm();
static uint64_t enhenced_clock_algrithm();
static int64_t file_read(const char *path, uint64_t offset, uint64_t length, void *buffer);
static int64_t file_write(const char *path, uint64_t offset, uint64_t length, void *src);
static void swap_copy_in(uint64_t new_vaddr, int64_t swapfile_index);
static void swap_copy_out(uint64_t target_vaddr, int64_t swapfile_index);
static void swap_out(uint64_t vaddr);
int64_t find_empty_swap_page();

/** swap 页表，跟踪 swapfile 的所有页 */
unsigned char swap_map[SWAP_PAGES] = { 0 };
uint8_t swap_file[SWAP_SIZE]; //TEMP

static uint64_t clock_algrithm() {
    uint64_t *start_vaddr = &(current->swap_info.clock_info.vaddr);
    // uint64_t last_start_addr = *start_vaddr;
    uint64_t vpn1, vpn2, vpn3;
    uint64_t has_annoymous_page = 0;
    uint64_t round_start, round_end;
    uint64_t half_round = 1;
    do {
        uint64_t last_pte = 0;
        round_start = half_round ? *start_vaddr : START_CODE;
        round_end= half_round ? KERNEL_ADDRESS : *start_vaddr;
        for (vpn1 = GET_VPN1(round_start); vpn1 < 0x200 && !last_pte; vpn1++) {
            for (vpn2 = GET_VPN2(round_start); vpn2 < 0x200 && !last_pte; vpn2++) {
                for (vpn3 = GET_VPN3(round_start); vpn3 < 0x200 && !last_pte; vpn3++) {
                    uint64_t vaddr = vpn1 << 30 | vpn2 << 21 | vpn3 << 12;
                    last_pte = vaddr == round_end;
                    uint64_t *pte = get_pte(vaddr);
                    if(!pte || !(*pte & PAGE_VALID)) continue;
                    // check ref, swap one ref(annoymous page) only
                    if (mem_map[MAP_NR(GET_PAGE_ADDR(*pte))] == 1) {
                        has_annoymous_page = 1;
                        if (*pte & PAGE_ACCESSED) {
                            *pte &= ~PAGE_ACCESSED;
                        } else {
                            *start_vaddr = vaddr;
                            return vaddr;
                        }
                    } // check ref
                } // for vpn3
            } // for vpn2
        } // for vpn1
        half_round = !half_round;
    } while(has_annoymous_page || !half_round /* 还没有走完一圈 */);
    return 0;    // 没有匿名页
}

static uint64_t enhenced_clock_algrithm() {
    uint64_t *start_vaddr = &(current->swap_info.clock_info.vaddr);
    // uint64_t last_start_addr = *start_vaddr;
    uint64_t vpn1, vpn2, vpn3;
    uint64_t has_annoymous_page = 0;
    uint64_t round_start, round_end;
    uint64_t half_round = 1;
    uint64_t stage = 0;
    do {
        uint64_t last_pte = 0;
        round_start = half_round ? *start_vaddr : START_CODE;
        round_end= half_round ? KERNEL_ADDRESS : *start_vaddr;
        for (vpn1 = GET_VPN1(round_start); vpn1 < 0x200 && !last_pte; vpn1++) {
            for (vpn2 = GET_VPN2(round_start); vpn2 < 0x200 && !last_pte; vpn2++) {
                for (vpn3 = GET_VPN3(round_start); vpn3 < 0x200 && !last_pte; vpn3++) {
                    uint64_t vaddr = vpn1 << 30 | vpn2 << 21 | vpn3 << 12;
                    last_pte = vaddr >= round_end;
                    volatile uint64_t *pte = get_pte(vaddr);
                    if(!pte || !(*pte & PAGE_VALID)) continue;
                    // check ref, swap one ref(annoymous page) only
                    if (mem_map[MAP_NR(GET_PAGE_ADDR(*pte))] == 1) {
                        has_annoymous_page = 1;
                        if (stage == 0){
                            if (!(*pte & PAGE_DIRTY) && !(*pte & PAGE_ACCESSED)){
                                *start_vaddr = vaddr;
                                return vaddr;
                            }
                        }
                        else{
                            if (*pte & PAGE_ACCESSED){
                                *pte &= ~PAGE_ACCESSED;
                                __asm__ __volatile__("fence\n\t":::"memory");
                                kprintf("%x\n",*pte&PAGE_ACCESSED);
                            }
                            else{
                                if (*pte & PAGE_DIRTY){
                                    *start_vaddr = vaddr;
                                    return vaddr;
                                }
                            }
                        }
                    } // check ref
                } // for vpn3
            } // for vpn2
        } // for vpn1
        if (!half_round) stage = !stage;
        half_round = !half_round;
    } while(has_annoymous_page || !half_round /* 还没有走完一圈 */);
    return 0;    // 没有匿名页
}

void swap_init(){
    memset(swap_file,0,SWAP_SIZE);
}

static int64_t file_read(const char *path, uint64_t offset, uint64_t length, void *buffer){
    if (strcmp(path, "swap")) return -ENOENT;
    memcpy(buffer, &swap_file[offset*PAGE_SIZE], PAGE_SIZE);
    return PAGE_SIZE;
}

static int64_t file_write(const char *path, uint64_t offset, uint64_t length, void *src){
    if (strcmp(path, "swap")) return -ENOENT;
    memcpy(&swap_file[offset*PAGE_SIZE], src, PAGE_SIZE);
    return PAGE_SIZE;
}

static void swap_copy_in(uint64_t new_vaddr, int64_t swapfile_index) {
    int ret = file_read("swap", swapfile_index * PAGE_SIZE, PAGE_SIZE, (char *)new_vaddr);
    if (ret < 0) {
        panic("swap 文件读取错误");
    } else if (ret != PAGE_SIZE) {
        panic("读取 swap 文件时意外地遇到文件尾");
    }
}

/** 找到一个可用的 swap_map 项
 * @return 可用的 swap_map 项的下标
 **/
int64_t find_empty_swap_page()
{
    uint64_t index;
    for (index = 0; index < SWAP_PAGES; index++)
    {
        if (!swap_map[index])
            return index;
    }
    return -1;
}

static void swap_copy_out(uint64_t target_vaddr, int64_t swapfile_index)
{
    int ret = file_write("swap", swapfile_index * PAGE_SIZE, PAGE_SIZE, (char *)target_vaddr);
    if (ret < 0) {
        panic("swap 文件读取错误");
    } else if (ret != PAGE_SIZE) {
        panic("读取 swap 文件时意外地遇到文件尾");
    }
}

/** 换入指定的页
 * @note 不需要对齐
 **/
void swap_in(uint64_t vaddr){
    vaddr = FLOOR(vaddr);
    uint64_t *pte = get_pte(vaddr);
    uint64_t swapfile_index = GET_PAGE_ADDR_BITS(*pte);

    uint64_t new_page_paddr = get_free_page();
    SET_PAGE_ADDR(*pte, new_page_paddr);
    *pte |= PAGE_VALID;
    swap_copy_in(vaddr, swapfile_index);

    // mem_map[MAP_NR(new_page_paddr)] += 1;
    swap_map[swapfile_index] -= 1;
    
}

/** 换出指定的页 **/
static void swap_out(uint64_t vaddr){
    uint64_t *pte = get_pte(vaddr);
    uint64_t swapout_paddr = GET_PAGE_ADDR(*pte);

    int64_t empty_swap_page = find_empty_swap_page();
    if (empty_swap_page == -1)
        panic("找不到可用的 swap 页");  // 假设在没有可用物理内存时才换出。也可以考虑杀进程，或什么也不做（如果设置了换出意愿，物理内存有空间但还是想换出）

    swap_copy_out(vaddr, empty_swap_page);

    *pte &= ~PAGE_VALID;
    SET_PAGE_ADDR_BITS(*pte, empty_swap_page);

    swap_map[empty_swap_page] = 1;
    mem_map[MAP_NR(swapout_paddr)] = 0;
}

/** 执行一次换出，换出一页 **/
void do_swap_out(){
    uint64_t vaddr = enhenced_clock_algrithm();
    if (vaddr) {
        swap_out(vaddr);
        kprintf("swapped out one page @ %p.\n", vaddr);
    } else kputs("swap out failed.");
}
