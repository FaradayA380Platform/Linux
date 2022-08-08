/**
 *  drivers/clk/faraday/clk-tc12ngrc.c
 *
 *  Faraday TC12NGRC Clock Tree
 *
 *  Copyright (C) 2019 Faraday Technology
 *  Copyright (C) 2019 Bo-Cun Chen <bcchen@faraday-tech.com>
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

#define MACH_TC12NGRC_VP

struct tc12ngrc_clk {
	char *name;
	char *parent;
	unsigned long rate;
	unsigned int mult;
	unsigned int div;
};

static struct tc12ngrc_clk tc12ngrc_fixed_rate_clk[] = {
	{
		.name = "osc0",
		.rate = 25000000,
	},
};

static struct tc12ngrc_clk tc12ngrc_pll_clk[] = {
	{
		.name = "pll0",
		.parent = "osc0",
		.div = 1,
	},
};

static struct tc12ngrc_clk tc12ngrc_fixed_factor_clk[] = {
	{
		.name = "AHB",
		.parent = "pll0",
		.mult = 1,
		.div = 4,
	}, {
		.name = "ahb",
		.parent = "pll0",
		.mult = 1,
		.div = 4,
	}, {
		.name = "hclk",
		.parent = "pll0",
		.mult = 1,
		.div = 4,
	}, {
		.name = "APB",
		.parent = "pll0",
		.mult = 1,
		.div = 8,
	}, {
		.name = "apb",
		.parent = "pll0",
		.mult = 1,
		.div = 8,
	}, {
		.name = "pclk",
		.parent = "pll0",
		.mult = 1,
		.div = 8,
	}, {
		.name = "cpu",
		.parent = "pll0",
		.mult = 1,
		.div = 2,
	}, {
		.name = "ddrmclk",
		.parent = "pll0",
		.mult = 1,
		.div = 1,
	}, {
		.name = "spiclk",
		.parent = "pll0",
		.mult = 1,
		.div = 2,
	},
};


unsigned int tc12ngrc_clk_get_pll_ns(const char *name)
{
	void __iomem *scu_va;
	unsigned int ns = 0;

	scu_va = ioremap(PLAT_FTSCU100_BASE, SZ_4K);

	if (!strcmp(name, "pll0")) {
#ifndef MACH_TC12NGRC_VP
		ns = (readl(scu_va + 0x30) >> 24) & 0xFF;
#else
		ns = 16;
#endif
	}

	iounmap(scu_va);

	return ns;
}

unsigned int tc12ngrc_clk_get_cpu_div(void)
{
	void __iomem *scu_va;
	unsigned int div;

	scu_va = ioremap(PLAT_FTSCU100_BASE, SZ_4K);

#ifndef MACH_TC12NGRC_VP
	switch ((readl(scu_va + 0x20) >> 16) & 0x3) {
	case 2:
		div = 8;
		break;
	case 1:
		div = 4;
		break;
	case 0:
	default:
		div = 2;
		break;
	}
#else
	div = 2;
#endif

	iounmap(scu_va);

	return div;
}

unsigned int tc12ngrc_clk_get_ahb_div(void)
{
	void __iomem *scu_va;
	unsigned int div;

	scu_va = ioremap(PLAT_FTSCU100_BASE, SZ_4K);

#ifndef MACH_TC12NGRC_VP
	switch ((readl(scu_va + 0x20) >> 20) & 0x3) {
	case 2:
		div = 16;
		break;
	case 1:
		div = 8;
		break;
	case 0:
	default:
		div = 4;
		break;
	}
#else
	div = 4;
#endif

	iounmap(scu_va);

	return div;
}

unsigned int tc12ngrc_clk_get_apb_div(void)
{
	void __iomem *scu_va;
	unsigned int div;

	scu_va = ioremap(PLAT_FTSCU100_BASE, SZ_4K);

#ifndef MACH_TC12NGRC_VP
	switch ((readl(scu_va + 0x20) >> 20) & 0x3) {
	case 2:
	case 1:
		div = 16;
		break;
	case 0:
	default:
		div = 8;
		break;
	}
#else
	div = 8;
#endif

	iounmap(scu_va);

	return div;
}

unsigned int tc12ngrc_clk_get_div(const char *name)
{
	unsigned int div = 1;

	if (!strcmp(name, "cpu")) {
		div = tc12ngrc_clk_get_cpu_div();
	} else if (!strcmp(name, "AHB") || !strcmp(name, "ahb") || !strcmp(name, "hclk") || \
	           !strcmp(name, "ddrmclk") || !strcmp(name, "spiclk")) {
		div = tc12ngrc_clk_get_ahb_div();
	} else if (!strcmp(name, "APB") || !strcmp(name, "apb") || !strcmp(name, "pclk")) {
		div = tc12ngrc_clk_get_apb_div();
	}

	return div;
}

int __init tc12ngrc_clocks_init(void)
{
	struct clk *clk;
	const char *name, *parent;
	unsigned int i, mult, div;
	unsigned long cpuclk, hclk, pclk, mclk;

	for (i = 0; i < ARRAY_SIZE(tc12ngrc_fixed_rate_clk); i++) {
		name = tc12ngrc_fixed_rate_clk[i].name;
		clk = clk_register_fixed_rate(NULL, name, NULL,
				0, tc12ngrc_fixed_rate_clk[i].rate);
		clk_register_clkdev(clk, name, NULL);
		clk_prepare_enable(clk);
	}

	for (i = 0; i < ARRAY_SIZE(tc12ngrc_pll_clk); i++) {
		name = tc12ngrc_pll_clk[i].name;
		parent = tc12ngrc_pll_clk[i].parent;

		mult = tc12ngrc_clk_get_pll_ns(name);
		div = tc12ngrc_pll_clk[i].div;
		clk = clk_register_fixed_factor(NULL, name,
				parent, 0, mult, div);
		clk_register_clkdev(clk, name, NULL);
		clk_prepare_enable(clk);
	}

	for (i = 0; i < ARRAY_SIZE(tc12ngrc_fixed_factor_clk); i++) {
		name = tc12ngrc_fixed_factor_clk[i].name;
		parent = tc12ngrc_fixed_factor_clk[i].parent;

		mult = tc12ngrc_fixed_factor_clk[i].mult;
		div = tc12ngrc_clk_get_div(name);
		clk = clk_register_fixed_factor(NULL, name,
				parent, 0, mult, div);
		clk_register_clkdev(clk, name, NULL);
		clk_prepare_enable(clk);
	}

	clk = clk_get(NULL, "cpu");
	cpuclk = clk_get_rate(clk);

	clk = clk_get(NULL, "hclk");
	hclk = clk_get_rate(clk);

	clk = clk_get(NULL, "pclk");
	pclk = clk_get_rate(clk);

	clk = clk_get(NULL, "ddrmclk");
	mclk = clk_get_rate(clk);

	printk(KERN_INFO "CPU: %ld MHz, DDR MCLK: %ld MHz, HCLK: %ld MHz, PCLK: %ld MHz\n",
	       cpuclk/1000/1000, mclk/1000/1000, hclk/1000/1000, pclk/1000/1000);

	return 0;
}

#ifdef CONFIG_OF
static void __init of_tc12ngrc_faraday_pll_setup(struct device_node *np)
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

	mult = tc12ngrc_clk_get_pll_ns(clk_name);

	parent_name = of_clk_get_parent_name(np, 0);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0,
					mult, div);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
	}
}

static void __init of_tc12ngrc_faraday_mux_setup(struct device_node *np)
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

	div = tc12ngrc_clk_get_div(clk_name);

	parent_name = of_clk_get_parent_name(np, 0);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0,
					mult, div);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
	}
}

static const __initconst struct of_device_id tc12ngrc_clk_match[] = {
	{ .compatible = "tc12ngrc,osc0", .data = of_fixed_clk_setup, },
	{ .compatible = "tc12ngrc,pll0", .data = of_tc12ngrc_faraday_pll_setup, },
	{ .compatible = "tc12ngrc,ahb", .data = of_tc12ngrc_faraday_mux_setup, },
	{ .compatible = "tc12ngrc,hclk", .data = of_tc12ngrc_faraday_mux_setup, },
	{ .compatible = "tc12ngrc,apb", .data = of_tc12ngrc_faraday_mux_setup, },
	{ .compatible = "tc12ngrc,pclk", .data = of_tc12ngrc_faraday_mux_setup, },
	{ .compatible = "tc12ngrc,cpu", .data = of_tc12ngrc_faraday_mux_setup, },
	{ .compatible = "tc12ngrc,ddrmclk", .data = of_tc12ngrc_faraday_mux_setup, },
	{ .compatible = "tc12ngrc,spiclk", .data = of_tc12ngrc_faraday_mux_setup, },
};

void __init of_tc12ngrc_clocks_init(struct device_node *n)
{
	struct device_node *node;
	struct of_phandle_args clkspec;
	struct clk *clk;
	unsigned long cpuclk, aclk, hclk, pclk, mclk;

	cpuclk = 0; aclk = 0; hclk = 0; pclk = 0; mclk = 0;

	of_clk_init(tc12ngrc_clk_match);

	for (node = of_find_matching_node(NULL, tc12ngrc_clk_match);
	     node; node = of_find_matching_node(node, tc12ngrc_clk_match)) {
		clkspec.np = node;
		clk = of_clk_get_from_provider(&clkspec);
		of_node_put(clkspec.np);

		if (!strcmp(__clk_get_name(clk), "cpu"))
			cpuclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "hclk"))
			hclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pclk"))
			pclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "ddrmclk"))
			mclk = clk_get_rate(clk);
	}

	printk(KERN_INFO "CPU: %ld MHz, DDR MCLK: %ld MHz, HCLK: %ld MHz, PCLK: %ld MHz\n",
	       cpuclk/1000/1000, mclk/1000/1000, hclk/1000/1000, pclk/1000/1000);
}
#endif
