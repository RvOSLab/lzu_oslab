#include <rtc.h>
#include <stddef.h>
#include <kdebug.h>

#define SUNXI_RTC_START_ADDR 0x07090000

#define SEC_PER_DAY 86400
#define SEC_PER_HOUR 3600
#define SEC_PER_MIN 60

struct sunxi_rtc_regs {
    uint32_t losc_ctrl_reg; // 0x00, losc control register
    uint32_t losc_auto_swt_sta_reg; // 0x04, losc auto switch status register
    uint32_t intosc_clk_prescal_reg; // 0x08, intosc clock prescaler register
    uint32_t padding1; // 0x0c, padding
    uint32_t rtc_day_reg; // 0x10, rtc day register
    uint32_t rtc_hh_mm_ss_reg; // 0x14, rtc hour, minute, second register
    uint32_t padding2[2]; // 0x18, padding
    uint32_t alarm0_day_set_reg; // 0x20, alarm0 day set register
    uint32_t alarm0_cur_vlu_reg; // 0x24, alarm0 current value register
    uint32_t alarm0_enable_reg; // 0x28, alarm0 enable register
    uint32_t alarm0_irq_en; // 0x2c, alarm0 interrupt enable register
    uint32_t alarm0_irq_sta_reg; // 0x30, alarm0 interrupt status register, Write 1 to clear
    uint32_t padding3[7]; // 0x34, padding
    uint32_t alarm_config_reg; // 0x50, alarm configuration register
};

static void timestamp_to_day_hh_mm_ss(uint64_t timestamp, uint64_t *day, uint64_t *hh, uint64_t *mm, uint64_t *ss)
{
    timestamp /= 1000000000; // ns -> s
    *day = timestamp / SEC_PER_DAY;
    *hh = timestamp % SEC_PER_DAY / SEC_PER_HOUR;
    *mm = timestamp % SEC_PER_HOUR / SEC_PER_MIN;
    *ss = timestamp % SEC_PER_MIN;
}

static uint64_t day_hh_mm_ss_to_timestamp(uint64_t day, uint64_t hh, uint64_t mm, uint64_t ss)
{
    uint64_t timestamp = day * SEC_PER_DAY + hh * SEC_PER_HOUR + mm * SEC_PER_MIN + ss;
    return timestamp * 1000000000; // s -> ns
}

uint64_t sunxi_rtc_read_time()
{
    volatile struct sunxi_rtc_regs *regs = (struct sunxi_rtc_regs *)SUNXI_RTC_START_ADDR;
    uint64_t day = regs->rtc_day_reg;
    uint64_t hh_mm_ss = regs->rtc_hh_mm_ss_reg;
    // 根据布局转换天、时、分、秒到时间戳
    return day_hh_mm_ss_to_timestamp(day, (hh_mm_ss >> 16) & 0x1f, (hh_mm_ss >> 8) & 0x3f, hh_mm_ss & 0x3f);
}

void sunxi_rtc_set_time(uint64_t now)
{
    uint64_t day;
    uint64_t hh;
    uint64_t mm;
    uint64_t ss;

    volatile struct sunxi_rtc_regs *regs = (struct sunxi_rtc_regs *)SUNXI_RTC_START_ADDR;

    // 转换时间戳到天、时、分、秒
    timestamp_to_day_hh_mm_ss(now, &day, &hh, &mm, &ss);
    // 首先检测 LOSC_CTRL_REG 的位 [8:7] 是否为 0
    if (regs->losc_ctrl_reg & 0x180) {
        // 如果不为0，报错
        kprintf("LOSC_CTRL_REG[8:7] is not 0, cannot set time\n");
        return;
    } else {
        now >>= 9; // ns -> s
        // 根据布局修改 RTC_DAY_REG
        regs->rtc_day_reg = (uint64_t)day;
        // 根据布局修改 RTC_HH_MM_SS_REG
        regs->rtc_hh_mm_ss_reg = hh << 16 | mm << 8 | ss;
    }
}

void sunxi_rtc_set_alarm(uint64_t alarm)
{
    uint64_t day;
    uint64_t hh;
    uint64_t mm;
    uint64_t ss;

    volatile struct sunxi_rtc_regs *regs = (struct sunxi_rtc_regs *)SUNXI_RTC_START_ADDR;
    // 通过写 ALARM0_IRQ_EN 启用 alram0 中断。
    regs->alarm0_irq_en = 1;
    // 设置时钟比较器：向 ALARM0_DAY_SET_REG 与 ALARM0_HH-MM-SS_SET_REG 写入闹钟的日、小时、分钟、秒。
    // 转换时间戳到天、时、分、秒
    timestamp_to_day_hh_mm_ss(alarm - 1000, &day, &hh, &mm, &ss);
    alarm >>= 9; // ns -> s
    // 根据布局修改 ALARM0_DAY_SET_REG
    regs->alarm0_day_set_reg = (uint64_t)day;
    // 根据布局修改 ALARM0_HH-MM-SS_SET_REG
    regs->alarm0_cur_vlu_reg = hh << 16 | mm << 8 | ss;
    // 写入 ALARM0_ENABLE_REG 以启用 alarm 0 功能
    regs->alarm0_enable_reg = 1;
}

uint64_t sunxi_rtc_read_alarm()
{
    volatile struct sunxi_rtc_regs *regs = (struct sunxi_rtc_regs *)SUNXI_RTC_START_ADDR;

    // 可以通过 ALARM0_DAY_SET_REG 和 ALARM0_HH-MM-SS_SET_REG (下图中为 ALARM_CUR_VLE_REG) 实时查询闹钟时间。
    uint64_t day = regs->alarm0_day_set_reg;
    uint64_t hh_mm_ss = regs->alarm0_cur_vlu_reg;
    return day_hh_mm_ss_to_timestamp(day, (hh_mm_ss >> 16) & 0x1f, (hh_mm_ss >> 8) & 0x3f, hh_mm_ss & 0x3f);
}

void sunxi_rtc_interrupt_handler()
{
    volatile struct sunxi_rtc_regs *regs = (struct sunxi_rtc_regs *)SUNXI_RTC_START_ADDR;
    // 在进入中断处理程序后，写入 ALARM0_IRQ_STA_REG 以清除中断，并执行中断处理程序。
    regs->alarm0_irq_sta_reg = 1;
}

void sunxi_rtc_clear_alarm()
{
    volatile struct sunxi_rtc_regs *regs = (struct sunxi_rtc_regs *)SUNXI_RTC_START_ADDR;
    regs->alarm0_irq_en = 0;
    regs->alarm0_irq_sta_reg = 1;
    regs->alarm0_enable_reg = 0;
}

static const struct rtc_class_ops sunxi_rtc_ops = {
    .read_time = sunxi_rtc_read_time,
    .set_time = sunxi_rtc_set_time,
    .read_alarm = sunxi_rtc_read_alarm,
    .set_alarm = sunxi_rtc_set_alarm,
    .rtc_interrupt_handler = sunxi_rtc_interrupt_handler,
    .clear_alarm = sunxi_rtc_clear_alarm,
};

extern struct rtc_class_device rtc_device;
void sunxi_rtc_init()
{
    rtc_device.ops = sunxi_rtc_ops;
}
