#include <mm.h>
#include <errno.h>
#include <swap.h>
#include <sched.h>
#include <assert.h>

static int64_t is_valid_but_fault(uint64_t *pte,uint64_t badvaddr, uint64_t cause, uint64_t spp);

static int64_t is_valid_but_fault(uint64_t *pte,uint64_t badvaddr, uint64_t cause, uint64_t spp){
    uint64_t op_flag = cause == CAUSE_LOAD_PAGE_FAULT ? PAGE_READABLE
                    : cause == CAUSE_STORE_PAGE_FAULT ? PAGE_WRITABLE
                    : PAGE_EXECUTABLE;
    if (*pte & op_flag) {   // 有操作权限
        if ((cause == CAUSE_STORE_PAGE_FAULT && (*pte & PAGE_ACCESSED) && (*pte & PAGE_DIRTY))
            || ((cause == CAUSE_LOAD_PAGE_FAULT || cause == CAUSE_INSTRUCTION_PAGE_FAULT) && (*pte & PAGE_ACCESSED))) { // 操作时（软件设置 A、D 位模式）已设置 A、D 位
            if (spp && (*pte & PAGE_USER))
                panic("Unknown page fault, may try to access USER memory when SUM unset.");
            else
                panic("Unknown page fault.");
        }
        *pte |= cause == CAUSE_STORE_PAGE_FAULT ? (PAGE_DIRTY | PAGE_ACCESSED) : PAGE_ACCESSED;  // 操作时（软件设置 A、D 位模式）未设置 A、D 位
        return 0;
    } else {    // 无操作权限
        if (*pte & PAGE_USER && cause == CAUSE_STORE_PAGE_FAULT && badvaddr >= current->start_data) {   // 用户页写时复制
            un_wp_page(pte);
            return 0;
        } else {    // 访问到无权地址
            return -EACCES;
        }
    }
}

int64_t page_fault_handler(uint64_t badvaddr, uint64_t cause, uint64_t spp) {
    uint64_t *pte = get_pte(badvaddr);
    if (pte && *pte != 0) { // 页表项存在 并且 不全为 0
        if (*pte & PAGE_USER) { // 是用户页
            if (*pte & PAGE_VALID) {    // 用户页且页有效
                return is_valid_but_fault(pte, badvaddr, cause, spp);
            } else {    // 用户页但页无效（被换出）
                swap_in(badvaddr);
                return 0;
            }
        } else {    // 访问内核页
            if (spp) {  // 内核态访问内核页
                if (*pte & PAGE_VALID) return is_valid_but_fault(pte, badvaddr, cause, spp);
                else panic("Unknown page fault.");
            } else return -EACCES;  // 用户态访问内核页
        }
    } else {    // 页表项不存在 或 页表项全为 0
        if (badvaddr >= current->end_data && badvaddr <= START_STACK) { // 在用户堆栈区
            get_empty_page(FLOOR(badvaddr), USER_RW);
            return 0;
        } else {    // 不在用户堆栈区
            return -EACCES;
        }
    }
    return -EACCES;
}
