/*
 *  Driver for Faraday GC1601 pin controller
 *
 *  Copyright (C) 2015 Faraday Technology
 *  Copyright (C) 2020 Faraday Linux Automation Tool
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
 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>

#include "pinctrl-ftscu010.h"

#define GC1601_PMUR_MFS0 0x81c
#define GC1601_PMUR_MFS1 0x840

/* Pins */
enum{
	GC1601_PIN_X_FA6_TCK,
	GC1601_PIN_X_FA6_TDI,
	GC1601_PIN_X_FA6_TDO,
	GC1601_PIN_X_FA6_TMS,
	GC1601_PIN_X_FA6_TRSTn,
	GC1601_PIN_X_FA6_DBGACK,
	GC1601_PIN_X_FA6_DBGRQ,
	GC1601_PIN_X_I2C0_SCL,
	GC1601_PIN_X_I2C0_SDA,
	GC1601_PIN_X_I2C1_SCL,
	GC1601_PIN_X_I2C1_SDA,
	GC1601_PIN_X_UART0_RX,
	GC1601_PIN_X_UART0_TX,
	GC1601_PIN_X_CLK8K_OUT,
	GC1601_PIN_X_TX_SD,
	GC1601_PIN_X_RX_LOS,
	GC1601_PIN_X_PON_BEN,
	GC1601_PIN_X_RX_PWRDOWN,
	GC1601_PIN_X_TX_PWRDOWN,
	GC1601_PIN_X_DYING_GASP,
	GC1601_PIN_X_DYING_OUT,
	GC1601_PIN_X_PTP_1PPS,
	GC1601_PIN_X_SYNCE_25M,
};

#define GC1601_PINCTRL_PIN(x) PINCTRL_PIN(GC1601_PIN_ ## x, #x)

static const struct pinctrl_pin_desc gc1601_pins[] = {
	GC1601_PINCTRL_PIN(X_FA6_TCK),
	GC1601_PINCTRL_PIN(X_FA6_TDI),
	GC1601_PINCTRL_PIN(X_FA6_TDO),
	GC1601_PINCTRL_PIN(X_FA6_TMS),
	GC1601_PINCTRL_PIN(X_FA6_TRSTn),
	GC1601_PINCTRL_PIN(X_FA6_DBGACK),
	GC1601_PINCTRL_PIN(X_FA6_DBGRQ),
	GC1601_PINCTRL_PIN(X_I2C0_SCL),
	GC1601_PINCTRL_PIN(X_I2C0_SDA),
	GC1601_PINCTRL_PIN(X_I2C1_SCL),
	GC1601_PINCTRL_PIN(X_I2C1_SDA),
	GC1601_PINCTRL_PIN(X_UART0_RX),
	GC1601_PINCTRL_PIN(X_UART0_TX),
	GC1601_PINCTRL_PIN(X_CLK8K_OUT),
	GC1601_PINCTRL_PIN(X_TX_SD),
	GC1601_PINCTRL_PIN(X_RX_LOS),
	GC1601_PINCTRL_PIN(X_PON_BEN),
	GC1601_PINCTRL_PIN(X_RX_PWRDOWN),
	GC1601_PINCTRL_PIN(X_TX_PWRDOWN),
	GC1601_PINCTRL_PIN(X_DYING_GASP),
	GC1601_PINCTRL_PIN(X_DYING_OUT),
	GC1601_PINCTRL_PIN(X_PTP_1PPS),
	GC1601_PINCTRL_PIN(X_SYNCE_25M),
};

/* Pin groups */
static const unsigned clk_gen_pins[] = {
	GC1601_PIN_X_FA6_DBGRQ,
	GC1601_PIN_X_SYNCE_25M,
};
static const unsigned fa626te55ee1001hh0lg_pins[] = {
	GC1601_PIN_X_FA6_DBGACK,
	GC1601_PIN_X_FA6_DBGRQ,
	GC1601_PIN_X_FA6_TCK,
	GC1601_PIN_X_FA6_TDI,
	GC1601_PIN_X_FA6_TDO,
	GC1601_PIN_X_FA6_TMS,
	GC1601_PIN_X_FA6_TRSTn,
};
static const unsigned ftgpio010_pins[] = {
	GC1601_PIN_X_CLK8K_OUT,
	GC1601_PIN_X_DYING_GASP,
	GC1601_PIN_X_DYING_OUT,
	GC1601_PIN_X_FA6_DBGACK,
	GC1601_PIN_X_FA6_DBGRQ,
	GC1601_PIN_X_FA6_TCK,
	GC1601_PIN_X_FA6_TDI,
	GC1601_PIN_X_FA6_TDO,
	GC1601_PIN_X_FA6_TMS,
	GC1601_PIN_X_FA6_TRSTn,
	GC1601_PIN_X_I2C0_SCL,
	GC1601_PIN_X_I2C0_SDA,
	GC1601_PIN_X_I2C1_SCL,
	GC1601_PIN_X_I2C1_SDA,
	GC1601_PIN_X_PON_BEN,
	GC1601_PIN_X_PTP_1PPS,
	GC1601_PIN_X_RX_LOS,
	GC1601_PIN_X_RX_PWRDOWN,
	GC1601_PIN_X_SYNCE_25M,
	GC1601_PIN_X_TX_PWRDOWN,
	GC1601_PIN_X_TX_SD,
	GC1601_PIN_X_UART0_RX,
	GC1601_PIN_X_UART0_TX,
};
static const unsigned ftiic010_pins[] = {
	GC1601_PIN_X_I2C0_SCL,
	GC1601_PIN_X_I2C0_SDA,
};
static const unsigned ftiic010_1_pins[] = {
	GC1601_PIN_X_I2C1_SCL,
	GC1601_PIN_X_I2C1_SDA,
};
static const unsigned ftuart010_pins[] = {
	GC1601_PIN_X_UART0_RX,
	GC1601_PIN_X_UART0_TX,
};
static const unsigned gc1601_top_pins[] = {
	GC1601_PIN_X_CLK8K_OUT,
	GC1601_PIN_X_FA6_DBGACK,
	GC1601_PIN_X_FA6_TCK,
	GC1601_PIN_X_FA6_TRSTn,
	GC1601_PIN_X_PON_BEN,
	GC1601_PIN_X_PTP_1PPS,
	GC1601_PIN_X_RX_LOS,
	GC1601_PIN_X_RX_PWRDOWN,
	GC1601_PIN_X_TX_PWRDOWN,
	GC1601_PIN_X_TX_SD,
};
static const unsigned sysc_pins[] = {
	GC1601_PIN_X_DYING_GASP,
	GC1601_PIN_X_DYING_OUT,
	GC1601_PIN_X_FA6_TDI,
	GC1601_PIN_X_FA6_TDO,
	GC1601_PIN_X_FA6_TMS,
};
static const unsigned na_pins[] = {
	GC1601_PIN_X_CLK8K_OUT,
	GC1601_PIN_X_DYING_GASP,
	GC1601_PIN_X_DYING_OUT,
	GC1601_PIN_X_FA6_DBGACK,
	GC1601_PIN_X_FA6_DBGRQ,
	GC1601_PIN_X_FA6_TCK,
	GC1601_PIN_X_FA6_TDI,
	GC1601_PIN_X_FA6_TDO,
	GC1601_PIN_X_FA6_TMS,
	GC1601_PIN_X_FA6_TRSTn,
	GC1601_PIN_X_I2C0_SCL,
	GC1601_PIN_X_I2C0_SDA,
	GC1601_PIN_X_I2C1_SCL,
	GC1601_PIN_X_I2C1_SDA,
	GC1601_PIN_X_PON_BEN,
	GC1601_PIN_X_PTP_1PPS,
	GC1601_PIN_X_RX_LOS,
	GC1601_PIN_X_RX_PWRDOWN,
	GC1601_PIN_X_SYNCE_25M,
	GC1601_PIN_X_TX_PWRDOWN,
	GC1601_PIN_X_TX_SD,
	GC1601_PIN_X_UART0_RX,
	GC1601_PIN_X_UART0_TX,
};

#define GROUP(gname)					\
	{						\
		.name = #gname,				\
		.pins = gname##_pins,			\
		.npins = ARRAY_SIZE(gname##_pins),	\
	}
	
static const struct ftscu010_pin_group gc1601_groups[] = {
	GROUP(clk_gen),
	GROUP(fa626te55ee1001hh0lg),
	GROUP(ftgpio010),
	GROUP(ftiic010),
	GROUP(ftiic010_1),
	GROUP(ftuart010),
	GROUP(gc1601_top),
	GROUP(sysc),
	GROUP(na),
};

/* Pin function groups */
static const char * const clk_gen_groups[] = { "clk_gen" };
static const char * const fa626te55ee1001hh0lg_groups[] = { "fa626te55ee1001hh0lg" };
static const char * const ftgpio010_groups[] = { "ftgpio010" };
static const char * const ftiic010_groups[] = { "ftiic010" };
static const char * const ftiic010_1_groups[] = { "ftiic010_1" };
static const char * const ftuart010_groups[] = { "ftuart010" };
static const char * const gc1601_top_groups[] = { "gc1601_top" };
static const char * const sysc_groups[] = { "sysc" };
static const char * const na_groups[] = { "na" };

/* Mux functions */
enum{
	GC1601_MUX_CLK_GEN,
	GC1601_MUX_FA626TE55EE1001HH0LG,
	GC1601_MUX_FTGPIO010,
	GC1601_MUX_FTIIC010,
	GC1601_MUX_FTIIC010_1,
	GC1601_MUX_FTUART010,
	GC1601_MUX_GC1601_TOP,
	GC1601_MUX_SYSC,
	GC1601_MUX_NA,
};

#define FUNCTION(fname)					\
	{						\
		.name = #fname,				\
		.groups = fname##_groups,		\
		.ngroups = ARRAY_SIZE(fname##_groups),	\
	}

static const struct ftscu010_pmx_function gc1601_pmx_functions[] = {
	FUNCTION(clk_gen),
	FUNCTION(fa626te55ee1001hh0lg),
	FUNCTION(ftgpio010),
	FUNCTION(ftiic010),
	FUNCTION(ftiic010_1),
	FUNCTION(ftuart010),
	FUNCTION(gc1601_top),
	FUNCTION(sysc),
	FUNCTION(na),
};

#define GC1601_PIN(pin, reg, f0, f1, f2, f3)		\
	[GC1601_PIN_ ## pin] = {				\
			.offset = GC1601_PMUR_ ## reg,	\
			.functions = {			\
				GC1601_MUX_ ## f0,	\
				GC1601_MUX_ ## f1,	\
				GC1601_MUX_ ## f2,	\
				GC1601_MUX_ ## f3,	\
			},				\
		}

static struct ftscu010_pin gc1601_pinmux_map[] = {
	/*			pin			reg		f0			f1			f2			f3	*/
	GC1601_PIN(X_FA6_TCK,       MFS0,   FA626TE55EE1001HH0LG,   GC1601_TOP, FTGPIO010, NA),
	GC1601_PIN(X_FA6_TDI,       MFS0,   FA626TE55EE1001HH0LG,   SYSC,       FTGPIO010, NA),
	GC1601_PIN(X_FA6_TDO,       MFS0,   FA626TE55EE1001HH0LG,   SYSC,       FTGPIO010, NA),
	GC1601_PIN(X_FA6_TMS,       MFS0,   FA626TE55EE1001HH0LG,   SYSC,       FTGPIO010, NA),
	GC1601_PIN(X_FA6_TRSTn,     MFS0,   FA626TE55EE1001HH0LG,   GC1601_TOP, FTGPIO010, NA),
	GC1601_PIN(X_FA6_DBGACK,    MFS0,   FA626TE55EE1001HH0LG,   GC1601_TOP, FTGPIO010, NA),
	GC1601_PIN(X_FA6_DBGRQ,     MFS0,   FA626TE55EE1001HH0LG,   CLK_GEN,    FTGPIO010, NA),
	GC1601_PIN(X_I2C0_SCL,      MFS0,   FTIIC010,               NA,         FTGPIO010, NA),
	GC1601_PIN(X_I2C0_SDA,      MFS0,   FTIIC010,               NA,         FTGPIO010, NA),
	GC1601_PIN(X_I2C1_SCL,      MFS0,   FTIIC010_1,             NA,         FTGPIO010, NA),
	GC1601_PIN(X_I2C1_SDA,      MFS0,   FTIIC010_1,             NA,         FTGPIO010, NA),
	GC1601_PIN(X_UART0_RX,      MFS0,   FTUART010,              NA,         FTGPIO010, NA),
	GC1601_PIN(X_UART0_TX,      MFS0,   FTUART010,              NA,         FTGPIO010, NA),
	GC1601_PIN(X_CLK8K_OUT,     MFS0,   GC1601_TOP,             NA,         FTGPIO010, NA),
	GC1601_PIN(X_TX_SD,         MFS0,   GC1601_TOP,             NA,         FTGPIO010, NA),
	GC1601_PIN(X_RX_LOS,        MFS0,   GC1601_TOP,             NA,         FTGPIO010, NA),
	GC1601_PIN(X_PON_BEN,       MFS1,   GC1601_TOP,             NA,         FTGPIO010, NA),
	GC1601_PIN(X_RX_PWRDOWN,    MFS1,   GC1601_TOP,             NA,         FTGPIO010, NA),
	GC1601_PIN(X_TX_PWRDOWN,    MFS1,   GC1601_TOP,             NA,         FTGPIO010, NA),
	GC1601_PIN(X_DYING_GASP,    MFS1,   SYSC,                   NA,         FTGPIO010, NA),
	GC1601_PIN(X_DYING_OUT,     MFS1,   SYSC,                   NA,         FTGPIO010, NA),
	GC1601_PIN(X_PTP_1PPS,      MFS1,   GC1601_TOP,             NA,         FTGPIO010, NA),
	GC1601_PIN(X_SYNCE_25M,     MFS1,   CLK_GEN,                NA,         FTGPIO010, NA),
};

static const struct ftscu010_pinctrl_soc_data gc1601_pinctrl_soc_data = {
	.pins = gc1601_pins,
	.npins = ARRAY_SIZE(gc1601_pins),
	.functions = gc1601_pmx_functions,
	.nfunctions = ARRAY_SIZE(gc1601_pmx_functions),
	.groups = gc1601_groups,
	.ngroups = ARRAY_SIZE(gc1601_groups),
	.map = gc1601_pinmux_map,
};

static int gc1601_pinctrl_probe(struct platform_device *pdev)
{
	return ftscu010_pinctrl_probe(pdev, &gc1601_pinctrl_soc_data);
}

static struct of_device_id faraday_pinctrl_of_match[] = {
	{ .compatible = "ftscu010-pinmux", },
	{ },
};

static struct platform_driver gc1601_pinctrl_driver = {
	.driver = {
		.name = "pinctrl-gc1601",
		.owner = THIS_MODULE,
		.of_match_table = faraday_pinctrl_of_match,
	},
	.remove = ftscu010_pinctrl_remove,
};

static int __init gc1601_pinctrl_init(void)
{
	return platform_driver_probe(&gc1601_pinctrl_driver, gc1601_pinctrl_probe);
}
arch_initcall(gc1601_pinctrl_init);

static void __exit gc1601_pinctrl_exit(void)
{
	platform_driver_unregister(&gc1601_pinctrl_driver);
}
module_exit(gc1601_pinctrl_exit);

MODULE_AUTHOR("Bo-Cun Chen <bcchen@faraday-tech.com");
MODULE_DESCRIPTION("Faraday HGU pinctrl driver");
MODULE_LICENSE("GPL");