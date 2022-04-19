#include <mm.h>
#include <kdebug.h>
#include <swap.h>
#include <sched.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

static uint64_t *get_pte(uint64_t vaddr);
static uint64_t clock_algrithm();
static uint64_t enhenced_clock_algrithm();
static int64_t file_read(const char *path, uint64_t offset, uint64_t length, void *buffer);
static int64_t file_write(const char *path, uint64_t offset, uint64_t length, void *src);
static void swap_copy_in(uint64_t new_vaddr, uint64_t swapfile_index);
static uint64_t swap_copy_out(uint64_t target_vaddr);
static void swap_out(uint64_t vaddr);

/** swap 页表，跟踪 swapfile 的所有页 */
unsigned char swap_map[SWAP_PAGES] = { 0 };
uint8_t swap_file[SWAP_SIZE]; //TEMP

/**
 * @brief 查找给定虚拟地址的页表项
 *
 * @param vaddr   虚拟地址
 * @return 页表项的指针
 */
static uint64_t *get_pte(uint64_t vaddr) {
    uint64_t vpns[3] = { GET_VPN1(vaddr), GET_VPN2(vaddr), GET_VPN3(vaddr) };
    uint64_t *page_table = pg_dir;
    for (size_t level = 0; level < 2; ++level) {
        uint64_t idx = vpns[level];
        if (!(page_table[idx] & PAGE_VALID)) {
            return NULL;
        }
        page_table =
            (uint64_t *)VIRTUAL(GET_PAGE_ADDR(page_table[idx]));
    }
    return &page_table[vpns[2]];
}

static uint64_t clock_algrithm() {
    uint64_t *start_vaddr = &(current->swap_info.clock_info.vaddr);
    uint64_t last_start_addr = *start_vaddr;
    uint64_t vpn1, vpn2, vpn3;
    uint64_t has_annoymous_page = 0;
    uint64_t round_start, round_end;
    uint64_t half_round = 1;
    do {
        uint64_t last_pte = 0;
        round_start = half_round ? *start_vaddr : START_CODE;
        round_end= half_round ? KERNEL_ADDRESS : *start_vaddr;
        for (vpn1 = GET_VPN1(round_start); vpn1 < GET_VPN1(round_end); vpn1++) {
            for (vpn2 = GET_VPN2(round_start); vpn2 < GET_VPN2(round_end); vpn2++) {
                for (vpn3 = GET_VPN3(round_start); vpn3 < GET_VPN3(round_end); vpn3++) {
                    uint64_t addr = vpn1 << 30 | vpn2 << 21 | vpn3 << 12;
                    uint64_t *pte = get_pte(addr);
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
        half_round = ~half_round;
    } while(has_annoymous_page || !half_round /* 还没有走完一圈 */);
    return 0;    // 没有匿名页
}

static uint64_t enhenced_clock_algrithm() {
    uint64_t *start_vaddr = &(current->swap_info.clock_info.vaddr);
    uint64_t last_start_addr = *start_vaddr;
    int vpn1, vpn2, vpn3;
    uint64_t has_annoymous_page = 0;
    uint64_t round_start, round_end;
    uint64_t half_round = 1;
    uint64_t stage = 0;
    do {
        round_start = half_round ? *start_vaddr : START_CODE;
        round_end= half_round ? KERNEL_ADDRESS : *start_vaddr;
        for (vpn1 = GET_VPN1(round_start); vpn1 < GET_VPN1(round_end); vpn1++) {
            for (vpn2 = GET_VPN2(round_start); vpn2 < GET_VPN2(round_end); vpn2++) {
                for (vpn3 = GET_VPN3(round_start); vpn3 < GET_VPN3(round_end); vpn3++) {
                    uint64_t vaddr = vpn1 << 30 | vpn2 << 21 | vpn3 << 12;
                    uint64_t *pte = get_pte(vaddr);
                    if(!pte || !(*pte & PAGE_VALID)) continue;
                    // check ref, swap one ref(annoymous page) only
                    if (mem_map[MAP_NR(GET_PAGE_ADDR(*pte))] == 1) {
                        has_annoymous_page = 1;
                        if (*pte & PAGE_ACCESSED) {
                            *pte &= ~PAGE_ACCESSED;
                        } else {
                            if (stage == 0) {
                                if (!(*pte & PAGE_DIRTY)) {
                                    *start_vaddr = vaddr;
                                    return vaddr;
                                }
                            } else {
                                if (*pte & PAGE_DIRTY) {
                                    *start_vaddr = vaddr;
                                    return vaddr;
                                }
                            }

                        }
                    } // check ref
                } // for vpn3
            } // for vpn2
        } // for vpn1
        if (!half_round) stage = ~stage;
        half_round = ~half_round;
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

static void swap_copy_in(uint64_t new_vaddr, uint64_t swapfile_index) {
    int ret = file_read("swap", swapfile_index * PAGE_SIZE, PAGE_SIZE, (char *)new_vaddr);
    if (ret < 0) {
        panic("swap 文件读取错误");
    } else if (ret != PAGE_SIZE) {
        panic("读取 swap 文件时意外地遇到文件尾");
    }
    swap_map[swapfile_index] = 0;
}

static uint64_t swap_copy_out(uint64_t target_vaddr) {
    uint64_t index;
    for (index = 0; index < SWAP_PAGES; index++)
    {
        if (!swap_map[index]) break;
    }
    int ret = file_write("swap", index * PAGE_SIZE, PAGE_SIZE, (char *)target_vaddr);
    if (ret < 0) {
        panic("swap 文件读取错误");
    } else if (ret != PAGE_SIZE) {
        panic("读取 swap 文件时意外地遇到文件尾");
    }
    swap_map[index] = 1;
    return index;
}

/** 换入指定的页（表项） **/
void swap_in(uint64_t vaddr){
    vaddr = FLOOR(vaddr);
    uint64_t *pte = get_pte(vaddr);
    uint64_t new_page_paddr = get_free_page();
    uint64_t swapfile_index = GET_PAGE_ADDR(*pte);
    *pte = GET_FLAG(*pte);
    
    SET_PAGE_ADDR(*pte, new_page_paddr);
    *pte |= PAGE_VALID;
    swap_copy_in(vaddr, swapfile_index);
}

/** 换出指定的页（表项） **/
static void swap_out(uint64_t vaddr){
    uint64_t *pte = get_pte(vaddr);
    uint64_t swapout_paddr = GET_PAGE_ADDR(*pte);
    uint64_t swapfile_index = swap_copy_out(vaddr);

    *pte &= ~PAGE_VALID;
    SET_PAGE_ADDR(*pte, swapfile_index);
    free_page(swapout_paddr);
}

/** 执行一次换出，换出一页 **/
void do_swap_out(){
    uint64_t vaddr = clock_algrithm();
    if (vaddr) {
        swap_out(vaddr);
        kputs("swapped out one page.");
    } else kputs("swap out failed.");
}
