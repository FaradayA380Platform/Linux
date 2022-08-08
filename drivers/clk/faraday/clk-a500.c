/**
 *  drivers/clk/faraday/clk-a500.c
 *
 *  Faraday A500 Clock Tree
 *
 *  Copyright (C) 2021 Faraday Technology
 *  Copyright (C) 2021 Bo-Cun Chen <bcchen@faraday-tech.com>
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
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/of_address.h>

static void __iomem *clk_base;

unsigned int a500_clk_get_pll_ns(const char *name)
{
	unsigned int ns = 0;

	if (!strcmp(name, "pll0")) {
		ns = ((readl(clk_base + 0x0030) >> 24) & 0x0FF);
	} else if (!strcmp(name, "pll1")) {
		ns = ((readl(clk_base + 0x8330) >> 16) & 0x1FF);
	} else if (!strcmp(name, "pll2")) {
		ns = ((readl(clk_base + 0x8338) >> 16) & 0x1FF);
	} else if (!strcmp(name, "pll3")) {
		ns = ((readl(clk_base + 0x8340) >> 16) & 0x1FF);
	}

	return ns;
}

void a500_clk_get_peri_0_mux(struct device_node *np, const char **parent_name)
{
	unsigned int axic_0_mux, axic_0_bypass;
	unsigned int sel;

	axic_0_mux = ((readl(clk_base + 0x812c) >> 17) & 0x1);

	axic_0_bypass = ((readl(clk_base + 0x812c) >> 18) & 0x1);

	if (axic_0_bypass == 0) {
		if (axic_0_mux == 0) {
			sel = 1;
		} else {
			sel = 2;
		}
	} else {
		sel = 0;
	}

	*parent_name = of_clk_get_parent_name(np, sel);
}

void a500_clk_get_peri_1_mux(struct device_node *np, const char **parent_name)
{
	unsigned int axic_1_mux, axic_1_bypass;
	unsigned int sel;

	axic_1_mux = ((readl(clk_base + 0x812c) >> 20) & 0x1);

	axic_1_bypass = ((readl(clk_base + 0x812c) >> 21) & 0x1);

	if (axic_1_bypass == 0) {
		if (axic_1_mux == 0) {
			sel = 1;
		} else {
			sel = 2;
		}
	} else {
		sel = 0;
	}

	*parent_name = of_clk_get_parent_name(np, sel);
}

void a500_clk_get_sdclk_mux(struct device_node *np, const char **parent_name)
{
	unsigned int sel;

	sel = ((readl(clk_base + 0x812c) >> 7) & 0x1);

	*parent_name = of_clk_get_parent_name(np, sel);
}

void a500_clk_get_spiclk_mux(struct device_node *np, const char **parent_name)
{
	unsigned int sel;

	sel = ((readl(clk_base + 0x812c) >> 5) & 0x1);

	*parent_name = of_clk_get_parent_name(np, sel);
}

void a500_clk_get_mux(struct device_node *np, const char **parent_name)
{
	const char *name = np->name;

	if (!strcmp(name, "peri_0_clk")) {
		a500_clk_get_peri_0_mux(np, parent_name);
	} else if (!strcmp(name, "peri_1_clk")) {
		a500_clk_get_peri_1_mux(np, parent_name);
	} else if (!strcmp(name, "sdclk")) {
		a500_clk_get_sdclk_mux(np, parent_name);
	} else if (!strcmp(name, "spiclk")) {
		a500_clk_get_spiclk_mux(np, parent_name);
	}
}

#ifdef CONFIG_OF
static void __init of_a500_faraday_pll_setup(struct device_node *np)
{
	struct clk *clk;
	const char *clk_name = np->name;
	const char *parent_name;
	u32 div, mult;

	if (of_property_read_u32(np, "clock-div", &div)) {
		pr_err("%s Fixed factor clock <%s> must have a"\
			"clock-div property\n",
			__func__, np->name);
		return;
	}

	of_property_read_string(np, "clock-output-names", &clk_name);

	mult = a500_clk_get_pll_ns(clk_name);

	parent_name = of_clk_get_parent_name(np, 0);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0,
					mult, div);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
	}
}

static void __init of_a500_faraday_mux_setup(struct device_node *np)
{
	struct clk *clk;
	const char *clk_name = np->name;
	const char *parent_name;
	u32 div, mult;

	if (of_property_read_u32(np, "clock-mult", &mult)) {
		pr_err("%s Fixed factor clock <%s> must have a"\
			"clock-mult property\n",
			__func__, np->name);
		return;
	}

	if (of_property_read_u32(np, "clock-div", &div)) {
		pr_err("%s Fixed factor clock <%s> must have a"\
			"clock-div property\n",
			__func__, np->name);
		return;
	}

	of_property_read_string(np, "clock-output-names", &clk_name);

	a500_clk_get_mux(np, &parent_name);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0,
					mult, div);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
	}
}

static const __initconst struct of_device_id a500_clk_match[] = {
	{ .compatible = "a500,osc0", .data = of_fixed_clk_setup, },
	{ .compatible = "a500,pll0", .data = of_a500_faraday_pll_setup, },
	{ .compatible = "a500,pll1", .data = of_a500_faraday_pll_setup, },
	{ .compatible = "a500,pll2", .data = of_a500_faraday_pll_setup, },
	{ .compatible = "a500,pll3", .data = of_a500_faraday_pll_setup, },
	{ .compatible = "a500,cpu", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a500,sys_0_clk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a500,sys_1_clk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a500,sys_2_clk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "a500,peri_0_clk", .data = of_a500_faraday_mux_setup, },
	{ .compatible = "a500,peri_1_clk", .data = of_a500_faraday_mux_setup, },
	{ .compatible = "a500,sdclk", .data = of_a500_faraday_mux_setup, },
	{ .compatible = "a500,spiclk", .data = of_a500_faraday_mux_setup, },
	{ .compatible = "a500,lcclk", .data = of_fixed_factor_clk_setup, },
};

void __init of_a500_clocks_init(struct device_node *n)
{
	struct device_node *node;
	struct of_phandle_args clkspec;
	struct clk *clk;
	unsigned long pll0, pll1, pll2, pll3;
	unsigned long cpuclk, sys_0_clk, sys_1_clk, sys_2_clk, peri_0_clk, peri_1_clk;
	unsigned long sdclk, spiclk, lcclk;

	pll0 = 0; pll1 = 0; pll2 = 0; pll3 = 0;
	cpuclk = 0; sys_0_clk = 0; sys_1_clk = 0; sys_2_clk = 0; peri_0_clk = 0; peri_1_clk = 0;
	sdclk = 0; spiclk = 0; lcclk = 0;

	clk_base = of_iomap(n, 0);
	if (WARN_ON(!clk_base))
		return;

	of_clk_init(a500_clk_match);

	for (node = of_find_matching_node(NULL, a500_clk_match);
	     node; node = of_find_matching_node(node, a500_clk_match)) {
		clkspec.np = node;
		clk = of_clk_get_from_provider(&clkspec);
		of_node_put(clkspec.np);

		if (!strcmp(__clk_get_name(clk), "pll0"))
			pll0 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pll1"))
			pll1 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pll2"))
			pll2 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pll3"))
			pll3 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "cpu"))
			cpuclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "sys_0_clk"))
			sys_0_clk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "sys_1_clk"))
			sys_1_clk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "sys_2_clk"))
			sys_2_clk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "peri_0_clk"))
			peri_0_clk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "peri_1_clk"))
			peri_1_clk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "sdclk"))
			sdclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "spiclk"))
			spiclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "lcclk"))
			lcclk = clk_get_rate(clk);
	}

	printk(KERN_INFO "PLL0: %ld MHz, PLL1: %ld MHz, PLL2: %ld MHz, PLL3: %ld MHz\n",
	       pll0/1000/1000, pll1/1000/1000, pll2/1000/1000, pll3/1000/1000);
	printk(KERN_INFO "CPU: %ld MHz, SYS0: %ld MHz, SYS1: %ld MHz SYS2: %ld MHz, PERI0: %ld MHz, PERI1: %ld MHz\n",
	       cpuclk/1000/1000, sys_0_clk/1000/1000, sys_1_clk/1000/1000, sys_2_clk/1000/1000, peri_0_clk/1000/1000, peri_1_clk/1000/1000);
	printk(KERN_INFO "SDCLK: %ld MHz, SPICLK: %ld MHz, LCCLK: %ld MHz\n",
	       sdclk/1000/1000, spiclk/1000/1000, lcclk/1000/1000);
}
CLK_OF_DECLARE(a500_clocks_init, "faraday,a500-clk", of_a500_clocks_init);
#endif
