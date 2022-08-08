/*
 *  arch/arm/mach-gc1601/platform_dt.c
 *
 *  Copyright (C) 2020 Faraday Technology
 *  B.C. Chen <bcchen@faraday-tech.com>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>
#include <linux/platform_data/at24.h>
#ifdef CONFIG_CPU_HAS_GIC
#include <linux/irqchip/arm-gic.h>
#endif
#include <linux/mtd/partitions.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/serial_8250.h>
#include <linux/pinctrl/machine.h>
#include <linux/usb/phy.h>
#include <linux/input/matrix_keypad.h>
#include <linux/reboot.h>

#include <linux/platform_data/clk-faraday.h>

#include <asm/clkdev.h>
#include <asm/setup.h>
#include <asm/smp_twd.h>
#include <asm/memory.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>

#include <mach/hardware.h>

#include <plat/core.h>
#include <plat/faraday.h>
#include <plat/fttmr010.h>

#ifndef WDT_REG32
#define WDT_REG32(off)      *(volatile uint32_t __force*)(PLAT_FTWDT010_VA_BASE + (off))                                                
#endif

struct of_dev_auxdata plat_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("pinctrl-gc1601",        PLAT_SCU_BASE,          "pinctrl-gc1601",       NULL),
	OF_DEV_AUXDATA("faraday,ftwdt010",      PLAT_FTWDT010_BASE,     "ftwdt010.0",           NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_BASE,    "serial0",              NULL),
	OF_DEV_AUXDATA("faraday,ftgpio010",     PLAT_FTGPIO010_BASE,    "ftgpio010.0",          NULL),
	OF_DEV_AUXDATA("faraday,fti2c010",      PLAT_FTIIC010_BASE,     "fti2c010.0",           NULL),
	OF_DEV_AUXDATA("faraday,fti2c010",      PLAT_FTIIC010_1_BASE,   "fti2c010.1",           NULL),
	OF_DEV_AUXDATA("faraday,ftspi020",      PLAT_FTSPI020_BASE,     "ftspi020.0",           NULL),
	{}
};

/******************************************************************************
 * platform dependent functions
 *****************************************************************************/

static struct map_desc plat_io_desc[] __initdata = {
	{
		.virtual    = PLAT_SCU_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_SCU_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTINTC030_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTINTC030_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTWDT010_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTWDT010_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
};

static void __init platform_map_io(void)
{
	iotable_init((struct map_desc *)plat_io_desc, ARRAY_SIZE(plat_io_desc));
}

static void __init platform_init_early(void)
{
}

static void __init platform_init(void)
{
	of_platform_populate(NULL, of_default_bus_match_table, plat_auxdata_lookup, NULL);
}

void platform_reset(enum reboot_mode mode, const char *cmd)
{
	void __iomem *wdt_va;

#ifdef CONFIG_CPU_ARM926T
	uint32_t val = 0;
	/*
	 * Below assembly codes are copy from
	 * arm926_flush_kern_cache_all() and cpu_arm926_proc_fin() in proc-arm926.s.
	 */
	/* Clean and invalidate caches */
	asm volatile(
		/* test,clean,invalidate */
		"1:	mrc	p15, 0, r15, c7, c14, 3\n"
		"	bne 1b\n"
		/* invalidate I caches */
		"	mcr	p15, 0, %0, c7, c5, 0\n"
		/* drain WB */
		"	mcr	p15, 0, %0, c7, c10, 4\n"
		:: "r" (0));

	/* Turn off caching */
	asm volatile(
		/* ctrl register */
		"	mrc	p15, 0, %0, c1, c0, 0\n"
		/* ...i............ */
		"	bic	%0, %0, #0x1000\n"
		/* ............wca. */
		"	bic	%0, %0, #0x000e\n"
		/* disable caches */
		"	mcr	p15, 0, %0, c1, c0, 0\n"
		: "=&r"(val) :);

	/* Push out any further dirty data, and ensure cache is empty */
	asm volatile(
		/* test,clean,invalidate */
		"1:	mrc	p15, 0, r15, c7, c14, 3\n"
		"	bne 1b\n"
		/* invalidate I caches */
		"	mcr	p15, 0, %0, c7, c5, 0\n"
		/* drain WB */
		"	mcr	p15, 0, %0, c7, c10, 4\n"
		:: "r" (0));
#endif

	wdt_va = ioremap(PLAT_FTWDT010_BASE, SZ_4K);

	writel(0x00000000, wdt_va + 0x0c);
	writel(0x000003E8, wdt_va + 0x04);
	writel(0x00000003, wdt_va + 0x0c);	// system reset
	writel(0x00005AB9, wdt_va + 0x08);

	iounmap(wdt_va);
}

static int __init platform_late_init(void)
{
	return 0;
}
late_initcall(platform_late_init);

static const char *faraday_dt_match[] __initconst = {
	"arm,faraday-soc",
	NULL,
};

DT_MACHINE_START(FARADAY, "GC1601")
	.atag_offset  = 0x100,
	.dt_compat    = faraday_dt_match,
	.smp          = smp_ops(faraday_smp_ops),
	.map_io       = platform_map_io,
	.init_early   = platform_init_early,
	.init_machine = platform_init,
	.restart      = platform_reset,
MACHINE_END
