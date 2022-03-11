#ifndef SUNXI_SMHC_H
#define SUNXI_SMHC_H

#include <device/smhc.h>

#define SUNXI_SMHC_MAJOR 0xaabbccdd

struct sunxi_smhc_regs
{
    volatile uint32_t SMHC_CTRL;    // 0x00, Control Register
    volatile uint32_t SMHC_CLKDIV;  // 0x04, Clock Divider Register
    volatile uint32_t SMHC_TMOUT;   // 0x08, Timeout Register
    volatile uint32_t SMHC_CTYPE;   // 0x0C, Bus Width Register
    volatile uint32_t SMHC_BLKSIZ;  // 0x10, Block Size Register
    volatile uint32_t SMHC_BYTCNT;  // 0x14, Byte Count Register
    volatile uint32_t SMHC_CMD;     // 0x18, Command Register
    volatile uint32_t SMHC_CMDARG;  // 0x1C, Command Argument Register
    volatile uint32_t SMHC_RESP0;   // 0x20, Response Register 0
    volatile uint32_t SMHC_RESP1;   // 0x24, Response Register 1
    volatile uint32_t SMHC_RESP2;   // 0x28, Response Register 2
    volatile uint32_t SMHC_RESP3;   // 0x2C, Response Register 3
    volatile uint32_t SMHC_INTMASK; // 0x30, Interrupt Mask Register
    volatile uint32_t SMHC_MINTSTS; // 0x34, Masked Interrupt Status Register
    volatile uint32_t SMHC_RINTSTS; // 0x38, Raw Interrupt Status Register
    volatile uint32_t SMHC_STATUS;  // 0x3C, Status Register
    volatile uint32_t SMHC_FIFOTH;  // 0x40, FIFO Water Level Register
    volatile uint32_t SMHC_FUNS;    // 0x44, FIFO Function Select Register
    volatile uint32_t SMHC_TCBCNT;  // 0x48, Transferred Byte Count between Controller and Card
    volatile uint32_t SMHC_TBBCNT;  // 0x4C, Transferred Byte Count between Host Memory and Internal FIFO
    volatile uint32_t SMHC_DBGC;    // 0x50, Current Debug Control Register
    volatile uint32_t SMHC_CSDC;    // 0x54, CRC Status Detect Control Registers
    volatile uint32_t SMHC_A12A;    // 0x58, Auto Command 12 Argument Register
    volatile uint32_t SMHC_NTSR;    // 0x5C, SD New Timing Set Register
    volatile uint32_t padding[6];   // 0x60-0x74, Reserved
    volatile uint32_t SMHC_HWRST;   // 0x78, Hardware Reset Register
    volatile uint32_t SMHC_IDMAC;   // 0x80, IDMAC Control Register
    volatile uint32_t SMHC_DLBA;    // 0x84, Descriptor List Base Address Register
    volatile uint32_t SMHC_IDST;    // 0x88, IDMAC Status Register
    volatile uint32_t SMHC_IDIE;    // 0x8C, IDMAC Interrupt Enable Register
};

enum sunxi_smhc_reg_bit {
    SMHC0_RST = 16,
    SMHC0_GATING = 0,
    CCLK_ENB=16,
};

extern struct device_driver sunxi_smhc_driver;

#endif
