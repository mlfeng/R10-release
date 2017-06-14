/*
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk-provider.h>
#include <linux/clocksource.h>
#include <linux/cpuidle.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/irqchip.h>
#include <linux/kernel.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/rockchip/common.h>
#include <linux/rockchip/cpu.h>
#include <linux/rockchip/cpu_axi.h>
#include <linux/rockchip/cru.h>
#include <linux/rockchip/dvfs.h>
#include <linux/rockchip/grf.h>
#include <linux/rockchip/iomap.h>
#include <linux/rockchip/pmu.h>
#include <asm/cputype.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include "loader.h"
#define CPU 1108
#include "sram.h"
#include <linux/rockchip/cpu.h>
#include "pm.h"
#include "pm-rv1108.c"

#define RV1108_DEVICE(name) \
	{ \
		.virtual	= (unsigned long) RK_##name##_VIRT, \
		.pfn		= __phys_to_pfn(RV1108_##name##_PHYS), \
		.length		= RV1108_##name##_SIZE, \
		.type		= MT_DEVICE, \
	}

static struct map_desc rv1108_io_desc[] __initdata = {
	RV1108_DEVICE(CRU),
	RV1108_DEVICE(GRF),
	RV1108_DEVICE(TIMER),
	RV1108_DEVICE(EFUSE),
	RV1108_DEVICE(CPU_AXI_BUS),
#ifdef RV1108_EVB_V11
	RK_DEVICE(RK_DEBUG_UART_VIRT, RV1108_UART2_PHYS, RV1108_UART_SIZE),
#else
	RK_DEVICE(RK_DEBUG_UART_VIRT, RV1108_UART0_PHYS, RV1108_UART_SIZE),
#endif
	RK_DEVICE(RK_DDR_VIRT, RV1108_DDR_PCTL_PHYS, RV1108_DDR_PCTL_SIZE),
	RK_DEVICE(RK_DDR_VIRT + RV1108_DDR_PCTL_SIZE, RV1108_DDR_PHY_PHYS,
		  RV1108_DDR_PHY_SIZE),
	RK_DEVICE(RK_GPIO_VIRT(0), RV1108_GPIO0_PHYS, RV1108_GPIO_SIZE),
	RK_DEVICE(RK_GPIO_VIRT(1), RV1108_GPIO1_PHYS, RV1108_GPIO_SIZE),
	RK_DEVICE(RK_GPIO_VIRT(2), RV1108_GPIO2_PHYS, RV1108_GPIO_SIZE),
	RK_DEVICE(RK_GPIO_VIRT(3), RV1108_GPIO3_PHYS, RV1108_GPIO_SIZE),
	RK_DEVICE(RK_GIC_VIRT, RV1108_GIC_DIST_PHYS, RV1108_GIC_DIST_SIZE),
	RK_DEVICE(RK_GIC_VIRT + RV1108_GIC_DIST_SIZE, RV1108_GIC_CPU_PHYS,
		  RV1108_GIC_CPU_SIZE),
	RK_DEVICE(RK_PMU_GRF_VIRT, RV1108_PMU_GRF_PHYS, RV1108_PMU_GRF_SIZE),
	RK_DEVICE(RK_PMU_MEM_VIRT, RV1108_PMU_MEM_PHYS, RV1108_PMU_MEM_SIZE),
	RK_DEVICE(RK_PWM_VIRT, RV1108_PWM_PHYS, RV1108_PWM_SIZE),
	RK_DEVICE(RK_PMU_VIRT, RV1108_PMU_PHYS, RV1108_PMU_SIZE),
	RK_DEVICE(RK_SERVICE_MSCH_VIRT, RV1108_SERVICE_MSCH_PHYS,
			RV1108_SERVICE_MSCH_SIZE),
};

static void __init rv1108_boot_mode_init(void)
{
	u32 flag = readl_relaxed(RK_GRF_VIRT + RV1108_GRF_OS_REG0);
	u32 mode = readl_relaxed(RK_GRF_VIRT + RV1108_GRF_OS_REG1);
	u32 rst_st = readl_relaxed(RK_CRU_VIRT + RV1108_CRU_GLB_RST_ST);

	if (flag == (SYS_KERNRL_REBOOT_FLAG | BOOT_RECOVER))
		mode = BOOT_MODE_RECOVERY;
	if (rst_st & ((1 << 2) | (1 << 3)))
		mode = BOOT_MODE_WATCHDOG;

	rockchip_boot_mode_init(flag, mode);
}

static void __init rv110x_dt_map_io(void)
{
	iotable_init(rv1108_io_desc, ARRAY_SIZE(rv1108_io_desc));
	debug_ll_io_init();

	rv1108_boot_mode_init();
	rockchip_efuse_init();
}

static void __init rv1107_dt_map_io(void)
{
	rockchip_soc_id = ROCKCHIP_SOC_RV1107;
	rv110x_dt_map_io();
}

static void __init rv1108_dt_map_io(void)
{
	rockchip_soc_id = ROCKCHIP_SOC_RV1108;
	rv110x_dt_map_io();
}

static DEFINE_SPINLOCK(pmu_idle_lock);

static const u8 rv1108_pmu_idle_map[] = {
	[IDLE_REQ_DSP] = 2,
	[IDLE_REQ_CORE] = 3,
	[IDLE_REQ_BUS] = 4,
	[IDLE_REQ_PERI] = 6,
	[IDLE_REQ_VIDEO] = 7,
	[IDLE_REQ_VIO] = 8,
	[IDLE_REQ_MSCH] = 11,
	[IDLE_REQ_HEVC] = 12,
	[IDLE_REQ_PMU] = 13,
};

static int rv1108_set_idle_request(enum pmu_idle_req req, bool idle)
{
	u32 bit = rv1108_pmu_idle_map[req];
	u32 idle_mask, idle_target;
	u32 mask = BIT(bit);
	u32 val;
	unsigned long flags;

	if (req == IDLE_REQ_DSP)
		bit = bit + 3;

	idle_mask = BIT(bit) | BIT(bit + 16);
	idle_target = (idle << bit) | (idle << (bit + 16));

	spin_lock_irqsave(&pmu_idle_lock, flags);
	val = readl_relaxed(RK_PMU_VIRT + RV1108_PMU_IDLE_REQ);
	if (idle)
		val |=  mask;
	else
		val &= ~mask;
	writel_relaxed(val, RK_PMU_VIRT + RV1108_PMU_IDLE_REQ);

	dsb();

	while ((readl_relaxed(RK_PMU_VIRT + RV1108_PMU_IDLE_ST) & idle_mask) != idle_target)
		;

	spin_unlock_irqrestore(&pmu_idle_lock, flags);

	return 0;
}

static void __init rv1108_dt_init_timer(void)
{
	rockchip_pmu_ops.set_idle_request = rv1108_set_idle_request;
	of_clk_init(NULL);
	clocksource_of_init();
	of_dvfs_init();
}

static void __init rv1108_reserve(void)
{
	/* reserve memory for ION */
	rockchip_ion_reserve();
}

static void __init rv1108_init_late(void)
{
	if (rockchip_jtag_enabled)
		clk_prepare_enable(clk_get_sys(NULL, "clk_jtag"));
	rv1108_suspend_init();
	rockchip_suspend_init();
}

static void rv1108_restart(char mode, const char *cmd)
{
	u32 boot_flag, boot_mode;

	rockchip_restart_get_boot_mode(cmd, &boot_flag, &boot_mode);

	/* for loader */
	writel_relaxed(boot_flag, RK_GRF_VIRT + RV1108_GRF_OS_REG0);
	/* for linux */
	writel_relaxed(boot_mode, RK_GRF_VIRT + RV1108_GRF_OS_REG1);

	dsb();

	/* pll enter slow mode */
	writel_relaxed(0x01000000, RK_CRU_VIRT + RV1108_CRU_APLL_CON3);
	writel_relaxed(0x01000000, RK_CRU_VIRT + RV1108_CRU_GPLL_CON3);
	dsb();
	writel_relaxed(0xfdb9, RK_CRU_VIRT + RV1108_CRU_GLB_SRST_FST_VALUE);
	dsb();
}

static const char * const rv1108_dt_compat[] __initconst = {
	"rockchip,rv1108",
	NULL,
};

static const char * const rv1107_dt_compat[] __initconst = {
	"rockchip,rv1107",
	NULL,
};

DT_MACHINE_START(RV1108_DT, "Rockchip RV1108")
	.smp		= smp_ops(rockchip_smp_ops),
	.map_io		= rv1108_dt_map_io,
	.init_time	= rv1108_dt_init_timer,
	.dt_compat	= rv1108_dt_compat,
	.init_late	= rv1108_init_late,
	.reserve	= rv1108_reserve,
	.restart	= rv1108_restart,
MACHINE_END

DT_MACHINE_START(RV1107_DT, "Rockchip RV1107")
	.smp		= smp_ops(rockchip_smp_ops),
	.map_io		= rv1107_dt_map_io,
	.init_time	= rv1108_dt_init_timer,
	.dt_compat	= rv1107_dt_compat,
	.init_late	= rv1108_init_late,
	.reserve	= rv1108_reserve,
	.restart	= rv1108_restart,
MACHINE_END

char PIE_DATA(sram_stack)[1024];
EXPORT_PIE_SYMBOL(DATA(sram_stack));

static int __init rv1108_pie_init(void)
{
	int err;

	if (!cpu_is_rv110x())
		return 0;

	err = rockchip_pie_init();
	if (err)
		return err;

	rockchip_pie_chunk = pie_load_sections(rockchip_sram_pool, rk1108);
	if (IS_ERR(rockchip_pie_chunk)) {
		err = PTR_ERR(rockchip_pie_chunk);
		pr_err("%s: failed to load section %d\n", __func__, err);
		rockchip_pie_chunk = NULL;
		return err;
	}
	rockchip_sram_virt = kern_to_pie(rockchip_pie_chunk, &__pie_common_start[0]);
	rockchip_sram_stack = kern_to_pie(rockchip_pie_chunk, (char *)DATA(sram_stack) + sizeof(DATA(sram_stack)));

	return 0;
}
arch_initcall(rv1108_pie_init);

#include "ddr_rv1108.c"
static int __init rv1108_ddr_init(void)
{
	struct device_node *np = NULL;

	if (cpu_is_rv110x()) {
		ddr_change_freq = _ddr_change_freq;
		ddr_round_rate = _ddr_round_rate;
		ddr_set_auto_self_refresh = _ddr_set_auto_self_refresh;
		ddr_freq_scale_send_event = _ddr_freq_scale_send_event;
		np = of_find_node_by_name(np, "ddr_timing");
		ddr_init(0, np);
	}

	return 0;
}
arch_initcall_sync(rv1108_ddr_init);

