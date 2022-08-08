/*
 *  arch/arm/mach-tc12ngrc/platform_dt.c
 *
 *  Copyright (C) 2019 Faraday Technology
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

/*
#ifndef WDT_REG32
#define WDT_REG32(off)      *(volatile uint32_t __force*)(PLAT_FTWDT010_VA_BASE + (off))                                                
#endif
*/

static struct platform_nand_flash nand_fl = {
	.name = "nand",
};

struct of_dev_auxdata plat_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("pinctrl-a380",          PLAT_SCU_BASE,          "pinctrl-a380",         NULL),
	OF_DEV_AUXDATA("faraday,ftwdt010",      PLAT_FTWDT010_BASE,     "ftwdt010.0",           NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_BASE,    "serial0",              NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_1_BASE,  "serial1",              NULL),
	OF_DEV_AUXDATA("faraday,ftgpio010",     PLAT_FTGPIO010_BASE,    "ftgpio010.0",          NULL),
	OF_DEV_AUXDATA("faraday,ftdmac020",     PLAT_FTDMAC020_BASE,    "ftdmac020.0",          NULL),
	OF_DEV_AUXDATA("faraday,fotg210_hcd",   PLAT_FOTG210_BASE,      "fotg210_hcd.0",        NULL),
	OF_DEV_AUXDATA("faraday,fotg210_udc",   PLAT_FOTG210_1_BASE,      "fotg210_udc.0",        NULL),
	OF_DEV_AUXDATA("faraday,fti2c010",      PLAT_FTIIC010_BASE,     "fti2c010.0",           NULL),
	OF_DEV_AUXDATA("faraday,ftspi020",      PLAT_FTSPI020_BASE,     "ftspi020.0",           NULL),	
	OF_DEV_AUXDATA("faraday,ftgmac030",     PLAT_FTGMAC030_BASE,      "ftgmac030.0",               NULL),
	OF_DEV_AUXDATA("faraday,ftssp0xx-i2s",  PLAT_FTSSP010_BASE,     "ftssp0xx-i2s.0",       NULL),
	{}
};

/*
 * I2C devices
 */
#ifdef CONFIG_I2C_BOARDINFO

static struct at24_platform_data at24c16 = {
	.byte_len  = SZ_16K ,
	.page_size = 16,
};

static struct i2c_board_info __initdata i2c_devices[] = {
	{
		I2C_BOARD_INFO("at24", 0x50),   /* eeprom */
		.platform_data = &at24c16,
	},
	{
		I2C_BOARD_INFO("wm8731", 0x1b), /* audio codec*/
	},
};
#endif  /* #ifdef CONFIG_I2C_BOARDINFO */

/*
 * SPI devices
 */
#ifdef CONFIG_SPI_FTSSP010
static struct spi_board_info spi_devices[] __initdata = {
	{
		.modalias      = "spidev",
		.max_speed_hz  = 20 * 1000000,
		.bus_num       = 0,
		.chip_select   = 0,
		.mode          = SPI_MODE_3,
	}
};
#endif  /* #ifdef CONFIG_SPI_FTSSP010 */

/******************************************************************************
 * platform dependent functions
 *****************************************************************************/

/*static struct map_desc plat_io_desc[] __initdata = {
	{
		.virtual    = PLAT_SCU_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_SCU_BASE),
		.length     = SZ_8K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTINTC020_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTINTC020_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTUART010_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTUART010_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTUART010_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTUART010_1_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
};*/

static struct of_device_id faraday_clk_match[] __initconst = {
	{ .compatible = "faraday,tc12ngrc-clk", .data = of_tc12ngrc_clocks_init, },
	{}
};

static void __init platform_clock_init(void)
{
	struct device_node *np;
	const struct of_device_id *match;
	void (*clk_init)(struct device_node *);

	/* setup clock tree */
	np = of_find_matching_node(NULL, faraday_clk_match);
	if (!np)
		panic("unable to find a matching clock\n");

	match = of_match_node(faraday_clk_match, np);
	clk_init = match->data;
	clk_init(np);
}

static void __init platform_map_io(void)
{
//	iotable_init((struct map_desc *)plat_io_desc, ARRAY_SIZE(plat_io_desc));

//	ftscu100_init(__io(PLAT_SCU_VA_BASE));

//	platform_pinmux_init();

//	platform_clock_init();
}

static void __init platform_init_early(void)
{
	void __iomem *intc_va;

//	intc_va = ioremap(PLAT_FTINTC020_BASE, SZ_4K);

//	iounmap(intc_va);
}

static struct pinctrl_map __initdata platform_pinmux_default[] = {	
	PIN_MAP_MUX_GROUP("FTAHB020",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "ftahbb020",		"ftahbb020"),
	PIN_MAP_MUX_GROUP("FTAHBWRAP",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "ftahbwrap",		"ftahbwrap"),
	PIN_MAP_MUX_GROUP("ftdmac020.0",	PINCTRL_STATE_DEFAULT,	"pinctrl-a380", "ftdmac020",		"ftdmac020"),
	PIN_MAP_MUX_GROUP("ftgpio010.0",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "ftgpio010",		"ftgpio010"),
	PIN_MAP_MUX_GROUP("FTIIC010_0",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "fti2c010_0",		"fti2c010_0"),
	PIN_MAP_MUX_GROUP("FTIIC010_1",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "fti2c010_1",		"fti2c010_1"),
	PIN_MAP_MUX_GROUP("FTINTC030",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "ftintc030",		"ftintc030"),
	PIN_MAP_MUX_GROUP("ftkbc010.0",	PINCTRL_STATE_DEFAULT,	"pinctrl-a380", "ftkbc010",			"ftkbc010"),		
	PIN_MAP_MUX_GROUP("FTLCD210",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "ftlcd210",			"ftlcd210"),
	PIN_MAP_MUX_GROUP("FTLCD210TV",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "ftlcd210tv",		"ftlcd210tv"),
	PIN_MAP_MUX_GROUP("FTNAND024",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "ftnand024",		"ftnand024"),
	PIN_MAP_MUX_GROUP("FTSSP010_0",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "ftssp010_0",		"ftssp010_0"),	// For Audio
	PIN_MAP_MUX_GROUP("ftssp0xx-i2s.0",	PINCTRL_STATE_DEFAULT,	"pinctrl-a380", "ftssp010_1",		"ftssp010_1"),	// For Touch
	PIN_MAP_MUX_GROUP("FTSSP010_2",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "ftssp010_2",		"ftssp010_2"),
	PIN_MAP_MUX_GROUP("FTUART010_0",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "ftuart010_0_irda",	"ftuart010_0_irda"),
	PIN_MAP_MUX_GROUP("FTUART010_1",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "ftuart010_1",		"ftuart010_1"),
	PIN_MAP_MUX_GROUP("FTUART010_2",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "ftuart010_2",		"ftuart010_2"),
	PIN_MAP_MUX_GROUP("macb.0",		PINCTRL_STATE_DEFAULT,	"pinctrl-a380", "gbe",				"gbe"),	
	PIN_MAP_MUX_GROUP("VCAP300",	PINCTRL_STATE_SLEEP,	"pinctrl-a380", "vcap300",			"vcap300"),
};

static void __init platform_sys_timer_init(void)
{
	platform_clock_init();

	timer_probe();
}

static void __init platform_init(void)
{
#if 0
	pinctrl_register_mappings(platform_pinmux_default, ARRAY_SIZE(platform_pinmux_default));
#endif
#ifdef CONFIG_I2C_BOARDINFO
	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
#endif

#ifdef CONFIG_SPI_FTSSP010
	spi_register_board_info(spi_devices, ARRAY_SIZE(spi_devices));
#endif

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

DT_MACHINE_START(FARADAY, "TC12NGRC")
	.atag_offset  = 0x100,
	.dt_compat    = faraday_dt_match,
	.smp          = smp_ops(faraday_smp_ops),
	.map_io       = platform_map_io,
	.init_early   = platform_init_early,
	.init_time    = platform_sys_timer_init,
	.init_machine = platform_init,
	.restart      = platform_reset,
MACHINE_END