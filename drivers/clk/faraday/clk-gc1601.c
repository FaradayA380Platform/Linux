/**
 *  drivers/clk/faraday/clk-gc1601.c
 *
 *  Faraday GC1601 Clock Tree
 *
 *  Copyright (C) 2020 Faraday Technology
 *  Copyright (C) 2020 Bo-Cun Chen <bcchen@faraday-tech.com>
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

#include <mach/hardware.h>

struct gc1601_clk {
	char *name;
	char *parent;
	unsigned long rate;
	unsigned int mult;
	unsigned int div;
};

unsigned int gc1601_clk_get_pll_ns(const char *name)
{
	void __iomem *scu_va;
	unsigned int ns = 0;

	scu_va = ioremap(PLAT_FTSCU100_BASE, SZ_4K);

	if (!strcmp(name, "pll0_ckout1") || !strcmp(name, "pll0_ckout2") || !strcmp(name, "pll0_ckout3")) {
#ifndef CONFIG_MACH_GC1601_VP
		ns = ((readl(scu_va + 0x30) >> 24) & 0xFF) * 4;
#else
		ns = 64;
#endif
	}

	iounmap(scu_va);

	return ns;
}

unsigned int gc1601_clk_get_cpu_div(struct device_node *np, const char **parent_name)
{
	void __iomem *scu_va;
	unsigned int div;

	scu_va = ioremap(PLAT_FTSCU100_BASE, SZ_4K);

#ifndef CONFIG_MACH_GC1601_VP
	switch ((readl(scu_va + 0x20) >> 16) & 0x3) {
	case 2:
		div = 1;
		break;
	case 1:
		div = 2;
		break;
	case 0:
	default:
		div = 4;
		break;
	}
#else
	div = 1;
#endif

	*parent_name = of_clk_get_parent_name(np, 0);

	iounmap(scu_va);

	return div;
}

unsigned int gc1601_clk_get_axi_div(struct device_node *np, const char **parent_name)
{
	void __iomem *scu_va;
	unsigned int div;

	scu_va = ioremap(PLAT_FTSCU100_BASE, SZ_4K);

#ifndef CONFIG_MACH_GC1601_VP
	switch ((readl(scu_va + 0x20) >> 20) & 0x3) {
	case 1:
	default:
		div = 4;
		break;
	case 0:
		div = 8;
		break;
	}
#else
	div = 4;
#endif

	*parent_name = of_clk_get_parent_name(np, 0);

	iounmap(scu_va);

	return div;
}

unsigned int gc1601_clk_get_apb_div(struct device_node *np, const char **parent_name)
{
	void __iomem *scu_va;
	unsigned int div;

	scu_va = ioremap(PLAT_FTSCU100_BASE, SZ_4K);

#ifndef CONFIG_MACH_GC1601_VP
	switch ((readl(scu_va + 0x20) >> 20) & 0x3) {
	case 1:
		div = 4;
		break;
	case 0:
	default:
		div = 8;
		break;
	}
#else
	div = 4;
#endif

	*parent_name = of_clk_get_parent_name(np, 0);

	iounmap(scu_va);

	return div;
}

unsigned int gc1601_clk_get_ddrmclk_div(struct device_node *np, const char **parent_name)
{
	void __iomem *scu_va;
	unsigned int sel, div;

	scu_va = ioremap(PLAT_FTSCU100_BASE, SZ_4K);
	
#ifndef CONFIG_MACH_GC1601_VP
	sel = (readl(scu_va + 0x30) >> 4) & 0x3;
	switch (sel) {
		case 2:
		case 1:
		default:
			div = 1;
			break;
		case 0:
			div = 4;
			break;
	}
#else
	sel = 2;
	div = 1;
#endif

	*parent_name = of_clk_get_parent_name(np, sel);

	iounmap(scu_va);

	return div;
}

unsigned int gc1601_clk_get_spiclk_div(struct device_node *np, const char **parent_name)
{
	void __iomem *scu_va;
	unsigned int div;

	scu_va = ioremap(PLAT_FTSCU100_BASE, SZ_4K);

#ifndef CONFIG_MACH_GC1601_VP
	switch ((readl(scu_va + 0x810) >> 11) & 0x1) {
	case 1:
		div = 2;
		break;
	case 0:
	default:
		div = 8;
		break;
	}
#else
	div = 8;
#endif

	*parent_name = of_clk_get_parent_name(np, 0);

	iounmap(scu_va);

	return div;
}

unsigned int gc1601_clk_get_div(struct device_node *np, const char **parent_name)
{
	const char *name = np->name;
	unsigned int div = 1;

	if (!strcmp(name, "cpu")) {
		div = gc1601_clk_get_cpu_div(np, parent_name);
	} else if (!strcmp(name, "AXI") || !strcmp(name, "axi") || !strcmp(name, "aclk")) {
		div = gc1601_clk_get_axi_div(np, parent_name);
	} else if (!strcmp(name, "APB") || !strcmp(name, "apb") || !strcmp(name, "pclk")) {
		div = gc1601_clk_get_apb_div(np, parent_name);
	} else if (!strcmp(name, "ddrmclk")) {
		div = gc1601_clk_get_ddrmclk_div(np, parent_name);
	} else if (!strcmp(name, "spiclk")) {
		div = gc1601_clk_get_spiclk_div(np, parent_name);
	}

	return div;
}

#ifdef CONFIG_OF
static void __init of_gc1601_faraday_pll_setup(struct device_node *np)
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

	mult = gc1601_clk_get_pll_ns(clk_name);

	parent_name = of_clk_get_parent_name(np, 0);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0,
					mult, div);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
	}
}

static void __init of_gc1601_faraday_mux_setup(struct device_node *np)
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

	mult = 1;

	div = gc1601_clk_get_div(np, &parent_name);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0,
					mult, div);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
	}
}

static const __initconst struct of_device_id gc1601_clk_match[] = {
	{ .compatible = "gc1601,osc0", .data = of_fixed_clk_setup, },
	{ .compatible = "gc1601,pll0_ckout1", .data = of_gc1601_faraday_pll_setup, },
	{ .compatible = "gc1601,pll0_ckout2", .data = of_gc1601_faraday_pll_setup, },
	{ .compatible = "gc1601,pll0_ckout3", .data = of_gc1601_faraday_pll_setup, },
	{ .compatible = "gc1601,axi", .data = of_gc1601_faraday_mux_setup, },
	{ .compatible = "gc1601,aclk", .data = of_gc1601_faraday_mux_setup, },
	{ .compatible = "gc1601,apb", .data = of_gc1601_faraday_mux_setup, },
	{ .compatible = "gc1601,pclk", .data = of_gc1601_faraday_mux_setup, },
	{ .compatible = "gc1601,cpu", .data = of_gc1601_faraday_mux_setup, },
	{ .compatible = "gc1601,ddrmclk", .data = of_gc1601_faraday_mux_setup, },
	{ .compatible = "gc1601,spiclk", .data = of_gc1601_faraday_mux_setup, },
};

void __init of_gc1601_clocks_init(struct device_node *n)
{
	struct device_node *node;
	struct of_phandle_args clkspec;
	struct clk *clk;
	unsigned long cpuclk, aclk, pclk, mclk, spiclk;

	cpuclk = 0; aclk = 0; pclk = 0; mclk = 0; spiclk = 0;

	of_clk_init(gc1601_clk_match);

	for (node = of_find_matching_node(NULL, gc1601_clk_match);
	     node; node = of_find_matching_node(node, gc1601_clk_match)) {
		clkspec.np = node;
		clk = of_clk_get_from_provider(&clkspec);
		of_node_put(clkspec.np);

		if (!strcmp(__clk_get_name(clk), "cpu"))
			cpuclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "aclk"))
			aclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pclk"))
			pclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "ddrmclk"))
			mclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "spiclk"))
			spiclk = clk_get_rate(clk);
	}

	printk(KERN_INFO "CPU: %ld MHz, DDR MCLK: %ld MHz, ACLK: %ld MHz, PCLK: %ld MHz SPICLK: %ld MHz\n",
	       cpuclk/1000/1000, mclk/1000/1000, aclk/1000/1000, pclk/1000/1000, spiclk/1000/1000);
}
CLK_OF_DECLARE(of_gc1601_clocks_init, "faraday,gc1601-clk", of_gc1601_clocks_init);
#endif
