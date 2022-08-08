/**
 *  drivers/clk/faraday/clk-gc1610.c
 *
 *  Faraday GC1610 Clock Tree
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
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include <asm/io.h>

#include <dt-bindings/clock/clock-gc1610.h>

#include "clk.h"

static const struct faraday_fixed_rate_clock gc1610_fixed_rate_clks[] = {
	{ GC1610_FIXED_XCLK25M,    	"osc0",  			NULL,	25000000, CLK_IS_CRITICAL },
	{ GC1610_FIXED_TDMCLK24M,    "virt_pll3_clk",  	NULL,	24576000, CLK_IS_CRITICAL },
	{ GC1610_FIXED_ONUCLK155M,	"virt_pll4_clk",  	NULL,	155520000, CLK_IS_CRITICAL },
};

static const struct faraday_fixed_factor_clock gc1610_fixed_factor_clks[] = {
	{ GC1610_PLL0_CKOUT0B,       		"pll0_ckout0b",         		"osc0",				40,  		1,		CLK_SET_RATE_PARENT },				//250M
	{ GC1610_PLL0_CKOUT0B_DIV2,     	"pll0_ckout0b_div2",		"pll0_ckout0b",		1,		2,		CLK_SET_RATE_PARENT },
	{ GC1610_PLL0_CKOUT0B_DIV4,     	"pll0_ckout0b_div4",		"pll0_ckout0b",		1,		4,		CLK_SET_RATE_PARENT },
	{ GC1610_PLL1_U3_CLK2X,			"pll1_u3_clk2x",			"osc0",				84,		35,		CLK_SET_RATE_PARENT },
	{ GC1610_PLL1_U3_CLK1X,			"pll1_u3_clk1x",			"osc0",				84,		35*2,	CLK_SET_RATE_PARENT },
	{ GC1610_PLL1_U3_CLKLP,			"pll1_u3_clklp",			"osc0",				84,		35*4,	CLK_SET_RATE_PARENT },
	{ GC1610_PLL2_CKOUT1B,			"pll2_ckout1b",			"osc0",				10*2,	1, 		CLK_SET_RATE_PARENT },				
	{ GC1610_PLL2_CKOUT0B,			"pll2_ckout0b",			"osc0",				10*4,	1, 		CLK_SET_RATE_PARENT },
	{ GC1610_PLL2_CKOUT0B_DIV3	,	"pll2_ckout0b_333M",		"pll2_ckout0b_gck",		1,	3, 		CLK_SET_RATE_PARENT },
	{ GC1610_PLL2_CKOUT0B_DIV5	,	"pll2_ckout0b_200M",		"pll2_ckout0b_gck",		1,	5, 		CLK_SET_RATE_PARENT },
	{ GC1610_PLL2_CKOUT0B_DIV10,	"pll2_ckout0b_100M",		"pll2_ckout0b_gck",		1,	10, 		CLK_SET_RATE_PARENT },
	{ GC1610_PLL2_CKOUT2B,			"pll2_ckout2b",			"osc0",				10,		1, 		CLK_SET_RATE_PARENT },			//250M
	{ GC1610_PLL2_CKOUT2B_DIV2,		"pll2_ckout2b_gck_125M",	"pll2_ckout2b_gck",	1,		2, 		CLK_SET_RATE_PARENT },			//125M
	{ GC1610_PLL2_CKOUT2B_DIV4,		"pll2_ckout2b_gck_62.5M",	"pll2_ckout2b_gck",	1,		4, 		CLK_SET_RATE_PARENT },			//62.5M
	{ GC1610_PLL2_CKOUT2B_DIV8,		"pll2_ckout2b_gck_31.25M",	"pll2_ckout2b_gck",	1,		8,		 CLK_SET_RATE_PARENT },			//31.25M
	{ GC1610_PLL2_AXI4,  				"ace_clk",				"pll2_axi4_aclk_mux_out",1,	 	1,		CLK_SET_RATE_PARENT },
	{ GC1610_PLL2_AXI3,  				"axi_clk",					"pll2_axi3_aclk_mux_out",1,	 	1,		CLK_SET_RATE_PARENT },
	{ GC1610_PLL2_AHB,  				"ahb_clk",				"hclk_gck",			1,	 	1,		CLK_SET_RATE_PARENT },
	{ GC1610_PLL2_APB,  				"apb_clk",				"pclk_gck",			1,	 	1,		CLK_SET_RATE_PARENT },


	{ GC1610_TDC_XCLK,				"tdc_xclk",				"pll3_fout3",			1, 		1, 		CLK_SET_RATE_PARENT },
	
	{ GC1610_PLL3_FOUT3,				"pll3_fout3",				"virt_pll3_clk",			1, 		1, 		CLK_SET_RATE_PARENT },
	{ GC1610_PLL3_FOUT3_DIV8,		"pll3_fout3_1024K",		"pll3_fout3_gck",		1, 		8, 		CLK_SET_RATE_PARENT },	
	{ GC1610_PLL3_FOUT1,  			"pll3_fout1",				"virt_pll3_clk",			1, 		1, 		CLK_SET_RATE_PARENT },	
	{ GC1610_PLL3_FOUT1_DIV3,  		"pll3_fout1_8192K",		"pll3_fout1_gck",		1, 		3, 		CLK_SET_RATE_PARENT },		//8192K
	{ GC1610_PLL3_FOUT1_DIV6,  		"pll3_fout1_4096K",		"pll3_fout1_gck",		1, 		6, 		CLK_SET_RATE_PARENT },		//4096K
	{ GC1610_PLL3_FOUT1_DIV12,  		"pll3_fout1_2048K",		"pll3_fout1_gck",		1, 		12, 		CLK_SET_RATE_PARENT },		//2048K
//	{ GC1610_PLL4_FOUT1,  			"pll4_fout1",				"virt_pll4_clk",				10, 		1, 		CLK_SET_RATE_PARENT },		//155.52/156.25 M
	{ GC1610_PLL5_FOUT1,  			"pll5_fout1",				"osc0",				400/4, 	4*4, 	CLK_SET_RATE_PARENT },		//156.25M
	{ GC1610_PLL5_FOUT3,  			"pll5_fout3",				"osc0",				400/4,	2*5, 	CLK_SET_RATE_PARENT },		//250M
	{ GC1610_PLL5_FOUT2,  			"pll5_fout2",				"osc0",				400/4,	5*5,		CLK_SET_RATE_PARENT },		//100M
//	{ GC1610_PLL6_FOUT1,  			"pll6_fout1",				"osc0",				10*64, 	3,		CLK_SET_RATE_PARENT },		//533
};

static const char *const pll0_ca53_clkin_stg1_mux[] = { "pll0_ckout0b_div4_gck", 		"pll0_ckout0b_div2_gck" };												//ok
static const char *const pll0_ca53_clkin_stg2_mux[] = { "pll0_ca53_clkin_stg1_mux_out","pll0_ckout0b_gck" };													//ok
static const char *const pll2_axi4_aclk_mux[] = 		{ "pll2_ckout1b_gck", 			"pll2_ckout0b_333M",		"pll2_ckout2b_gck",	"pll2_ckout2b_gck_125M"};	//ok
static const char *const pll2_axi3_aclk_mux[] = 		{ "pll2_ckout2b_gck", 			"pll2_ckout2b_gck_125M",	"pll2_ckout2b_gck_62.5M",	"pll2_ckout2b_31.25M"}; //ok
static const char *const pll2_onu_250m_100m_mux[] ={ "pll2_ckout2b_gck", 			"pll2_ckout0b_100M"};													//ok
static const char *const pll2_onu_work_mux[] =		{ "pll2_ckout2b_gck_62.5M", 	"pll2_ckout2b_gck_125M",	"pll2_ckout0b_200M",	"pll2_ckout2b_gck"};//ok
static const char *const pll3_tdm_mux[] = 		{ "pll3_fout1_2048K", 			"pll3_fout1_4096K"};													//ok


//static const char *const ddr_clk_sel_mux[] = { "axi_clk_div2", "pll3_div2" };
static u32 mux_table_1bit[] = { 0, 1 };
static u32 mux_table_2bit[] = { 0, 1, 2, 3 };

static const struct faraday_mux_clock gc1610_mux_clks[] = {
	/* PLL Control Register (SCU) */
	{ PLL0_CA53_CLKIN_STG1_MUX, "pll0_ca53_clkin_stg1_mux_out", pll0_ca53_clkin_stg1_mux, ARRAY_SIZE(pll0_ca53_clkin_stg1_mux),		//ok
	  		CLK_SET_RATE_PARENT, 0x20, 16, 1, 0, mux_table_1bit },

	{ PLL0_CA53_CLKIN_STG2_MUX, "pll0_ca53_clkin_stg2_mux_out", pll0_ca53_clkin_stg2_mux, ARRAY_SIZE(pll0_ca53_clkin_stg2_mux),		//ok
	  		CLK_SET_RATE_PARENT, 0x20, 17, 1, 0, mux_table_1bit },

	{ PLL2_AXI4_ACLK_MUX, "pll2_axi4_aclk_mux_out", pll2_axi4_aclk_mux, ARRAY_SIZE(pll2_axi4_aclk_mux),								//ok
	  		CLK_SET_RATE_PARENT, 0x3c, 14, 2, 0, mux_table_2bit },

	{ PLL2_AXI3_ACLK_MUX, "pll2_axi3_aclk_mux_out", pll2_axi3_aclk_mux, ARRAY_SIZE(pll2_axi3_aclk_mux),								//ok
	  		CLK_SET_RATE_PARENT, 0x3c, 30, 2, 0, mux_table_2bit },

	{ PLL2_ONU_PMU_ACLK_MUX, "pll2_onu_pmu_aclk_mux_out", pll2_onu_250m_100m_mux, ARRAY_SIZE(pll2_onu_250m_100m_mux),		//ok
	  		CLK_SET_RATE_PARENT, 0x854, 0, 1, 0, mux_table_1bit },

	{ PLL2_PCLK_MUX, "pll2_pclk_mux_out", pll2_onu_work_mux, ARRAY_SIZE(pll2_onu_work_mux),										//ok
	  		CLK_SET_RATE_PARENT, 0x854, 5, 1, 0, mux_table_2bit },

	{ PLL3_TDM_MUX, "pll3_tdm_stg1_mux_out", pll3_tdm_mux, ARRAY_SIZE(pll3_tdm_mux),								//ok
	  		CLK_SET_RATE_PARENT, 0x854, 10, 1, 0, mux_table_1bit },

};

static const struct faraday_gate_clock gc1610_gate_busclks[] = {
	/* AHB Clock Control Register (SCU) */
	{ GC1610_USB2_EN,			"usb2_en",	"ahb_clk",         0,               0x00050, 0, 0 },
	{ GC1610_AES_EN,       		"aes_en",		"ahb_clk",         0,               0x00050, 1, 0 },
	{ GC1610_NANDC_EN,       		"nandc_en",	"ahb_clk",         0,               0x00050, 2, 0 },
	{ GC1610_SPACC_EN,     		"spacc_en",	"ahb_clk",         0,               0x00050, 3, 0 },
	{ GC1610_TDM_EN,     			"tdm_en",	"ahb_clk",         0,               0x00050, 4, 0 },

	/* APB Clock Control Register (SCU) */
	{ GC1610_EFUSE_PCLK_EN,		"efuse_pclk_en",      "apb_clk",         0,               0x00060, 0, 0 },
//	{ GC1610_EMC0_PCLK_EN,		"emc_0_pclk_en",      "apb_clk",         0,               0x00060, 1, 0 },
//	{ GC1610_EMC1_PCLK_EN,		"emc_1_pclk_en",      "apb_clk",         0,               0x00060, 2, 0 },
	{ GC1610_GPIO_PCLK_EN,		"gpio_pclk_en",      	"apb_clk",	0,               0x00060, 3, 0 },
	{ GC1610_I2C0_PCLK_EN,		"i2c_0_pclk_en",      	"apb_clk",	0,               0x00060, 4, 0 },
	{ GC1610_I2C1_PCLK_EN,		"i2c_1_pclk_en",      	"apb_clk",	0,               0x00060, 5, 0 },
	{ GC1610_MDIO_PCLK_EN,		"mdio_pclk_en",		"apb_clk",	0,               0x00060, 6, 0 },
	{ GC1610_SSP_PCLK_EN,		"ssp_pclk_en",      		"apb_clk",	0,               0x00060, 7, 0 },
	{ GC1610_TDC_PCLK_EN,		"tdc_pclk_en",      		"apb_clk",	0,               0x00060, 8, 0 },
//	{ GC1610_TMR0_PCLK_EN,		"tmr_0_pclk_en",      	"apb_clk",	0,               0x00060, 10, 0 },
//	{ GC1610_TMR1_PCLK_EN,		"tmr_1_pclk_en",      	"apb_clk",	0,               0x00060, 11, 0 },
//	{ GC1610_TMR2_PCLK_EN,		"tmr_2_pclk_en",      	"apb_clk",	0,               0x00060, 12, 0 },
//	{ GC1610_TMR3_PCLK_EN,		"tmr_3_pclk_en",		"apb_clk",	0,               0x00060, 13, 0 },
//	{ GC1610_UART0_PCLK_EN,		"uart_0_pclk_en",     	"apb_clk",	0,               0x00060, 13, 0 },
//	{ GC1610_UART1_PCLK_EN,		"uart_1_pclk_en",     	"apb_clk",	0,               0x00060, 14, 0 },
//	{ GC1610_UART2_PCLK_EN,		"uart_2_pclk_en",     	"apb_clk",	0,               0x00060, 15, 0 },
//	{ GC1610_UART3_PCLK_EN,		"uart_3_pclk_en",     	"apb_clk",	0,               0x00060, 16, 0 },
	{ GC1610_OTG210_PCLK_EN,	"usb2_pclk_en",		"apb_clk",	0,               0x00060, 17, 0 },
	{ GC1610_OTG330_PCLK_EN,	"usb3_pclk_en",		"apb_clk",	0,               0x00060, 18, 0 },
	{ GC1610_WDT0_PCLK_EN,		"wdt0_pclk_en",		"apb_clk",	0,               0x00060, 19, 0 },
//	{ reserved,					"reserved",			"reserved",	0,               0x00060, 20, 0 },
//	{ GC1610_DDR_PCLK_EN,		"ddr_pclk_en",		"apb_clk",	0,               0x00060, 21, 0 },
	{ GC1610_ONU_PCLK_EN,		"onu_pclk_en",		"apb_clk",	0,               0x00060, 22, 0 },
	{ GC1610_PCIE0_PCLK_EN,		"pcie_0_pclk_en",		"apb_clk",	0,               0x00060, 23, 0 },
	{ GC1610_PCIE1_PCLK_EN,		"pcie_1_pclk_en",		"apb_clk",	0,               0x00060, 24, 0 },
//	{ GC1610_SB_EXREG_PCLK_EN,	"sb_exreg_pclk_en",	"apb_clk",	0,               0x00060, 25, 0 },
//	{ GC1610_X2X0_PCLK_EN,		"x2x_0_pclk_en",		"apb_clk",	0,               0x00060, 26, 0 },
//	{ GC1610_X2X1_PCLK_EN,		"x2x_1_pclk_en",		"apb_clk",	0,               0x00060, 27, 0 },
//	{ GC1610_SRDS0_SIGPCLK_EN,	"serdes_0_sig_pclk_en","apb_clk",	0,               0x00060, 28, 0 },
//	{ GC1610_SRDS0_PCLK_EN,		"serdes_0_pclk_en",	"apb_clk",	0,               0x00060, 29, 0 },
//	{ GC1610_SRDS1_SIGPCLK_EN,	"serdes_1_sig_pclk_en","apb_clk",	0,               0x00060, 30, 0 },
//	{ GC1610_SRDS1_PCLK_EN,		"serdes_1_pclk_en",	"apb_clk",	0,               0x00060, 31, 0 },


	/* AXI Clock Control Register (SCU) */
	{ GC1610_GIC_ACLK_EN,		"gic_aclk_en",				"axi_clk",         CLK_IS_CRITICAL,               0x00080, 0, 0 },
//	{ GC1610_EMC0_ACLK_EN,		"emc_0_aclk_en",			"axi_clk",         0,               0x00080, 1, 0 },
//	{ GC1610_EMC1_ACLK_EN,		"emc_1_aclk_en",			"axi_clk",         0,               0x00080, 2, 0 },
	{ GC1610_SPI_ACLK_EN,		"spi_aclk_en",				"axi_clk",         0,               0x00080, 3, 0 },
	{ GC1610_OTG330_ACLK_EN,	"usb3_aclk_en",			"axi_clk",         0,               0x00080, 4, 0 },
	{ GC1610_X2P1_ACLK_EN,		"x2p_1_aclk_en",			"axi_clk",         0,               0x00080, 5, 0 },
	{ GC1610_ONUPMU_ACLK_EN,	"onu_pmu_aclk_en",		"axi_clk",         0,               0x00080, 6, 0 },
	{ GC1610_ONUWAC_ACLK_EN,	"onu_wac_aclk_en",		"axi_clk",         0,               0x00080, 7, 0 },
	{ GC1610_PCIE0_MST_ACLK_EN,	"pcie_0_mstr_aclk_en",		"axi_clk",         0,               0x00080, 8, 0 },
	{ GC1610_PCIE0_SLV_EN,		"pcie_0_slv_aclk_en",		"axi_clk",         0,               0x00080, 9, 0 },
	{ GC1610_PCIE1_MST_ACLK_EN,	"pcie_1_mstr_aclk_en",		"axi_clk",         0,               0x00080, 10, 0 },
	{ GC1610_PCIE1_SLV_EN,		"pcie_1_slv_aclk_en",		"axi_clk",         0,               0x00080, 11, 0 },
	{ GC1610_DDR_ACLK_EN,		"ddr_aclk_en",			"axi_clk",         CLK_IS_CRITICAL,               0x00080, 12, 0 },

};

static const struct faraday_gate_clock gc1610_gate_clks[] = {
	/* IP CLOCK ENABLE Register (EXT_REG) */

	{ GC1610_PLL0_CKOUT0B_EN,			"pll0_ckout0b_gck",       	"pll0_ckout0b",       			0,	0x0050, 20, 0 },
	{ GC1610_PLL0_CKOUT0B_DIV2_EN,       	"pll0_ckout0b_div2_gck",      "pll0_ckout0b_div2",       		0,	0x0050, 19, 0 },
	{ GC1610_PLL0_CKOUT0B_DIV4_EN,		"pll0_ckout0b_div4_gck",      "pll0_ckout0b_div4",       		0,	0x0050, 18, 0 },
	{ GC1610_CA53_CLKIN_EN,				"pll0_clkin_gck",       		"pll0_ca53_clkin_stg2_mux_out",	0,	0x0050, 17, 0 },
	{ GC1610_CA53_PCLKIN_EN,       		"pll0_pclkin_gck",      		"pll0_ckout0b_div4_gck",		0,	0x0050, 16, 0 },
	{ GC1610_PLL2_CKOUT1B_EN,       		"pll2_ckout1b_gck",       	"pll2_ckout1b",				0,	0x0050, 15, 0 },
	{ GC1610_PLL2_CKOUT0B_EN,       		"pll2_ckout0b_gck",       	"pll2_ckout0b",				0,	0x0050, 14, 0 },
	{ GC1610_PLL2_CKOUT2B_EN,       		"pll2_ckout2b_gck",       	"pll2_ckout2b",				0,	0x0050, 13, 0 },
	{ GC1610_AXI3_EN,					"axi3_aclk_gck",			"pll2_axi3_aclk_mux_out",		0,	0x0050, 12, 0 },
	{ GC1610_ONUPMU_EN,					"onup_mu_axi_aclk_gck",	"pll2_onu_pmu_aclk_mux_out",	0,	0x0050, 12, 0 },
	{ GC1610_HCLK_CK_EN, 				"hclk_gck", 				"pll2_ckout2b_gck_125M",         	0,	0x0050, 11, 0 },
	{ GC1610_PCLK_CK_EN, 				"pclk_gck",	  			"pll2_pclk_mux_out",           		0,	0x0050, 10, 0 },
	{ GC1610_PLL3_FOUT3_EN,				"pll3_fout3_gck",	  		"pll3_fout3",           			0,	0x0050, 10, 0 },
	{ GC1610_PLL3_FOUT1_EN, 				"pll3_fout1_gck",	  		"pll3_fout1",           			0,	0x0050, 10, 0 },
};

static int gc1610_clocks_probe(struct platform_device *pdev)
{
	struct faraday_clock_data *clk_data;
	struct resource *res;
	int ret;

	clk_data = faraday_clk_alloc(pdev, FARADAY_NR_CLKS);
	if (!clk_data)
		return -ENOMEM;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "scu");
	if (!res)
		return -ENODEV;
	clk_data->base = devm_ioremap(&pdev->dev,
	                              res->start, resource_size(res));
	if (!clk_data->base)
		return -EBUSY;

	ret = faraday_clk_register_fixed_rates(gc1610_fixed_rate_clks,
	                                       ARRAY_SIZE(gc1610_fixed_rate_clks),
	                                       clk_data);
	if (ret)
		return -EINVAL;

	ret = faraday_clk_register_fixed_factors(gc1610_fixed_factor_clks,
	                                         ARRAY_SIZE(gc1610_fixed_factor_clks),
	                                         clk_data);
	if (ret)
		goto unregister_fixed_factors;
	
	ret = faraday_clk_register_muxs(gc1610_mux_clks,
	                                ARRAY_SIZE(gc1610_mux_clks),
	                                clk_data->base,
	                                clk_data);
	if (ret)
		goto unregister_fixed_factors;

	ret = faraday_clk_register_gates(gc1610_gate_busclks,
	                                 ARRAY_SIZE(gc1610_gate_busclks),
	                                 clk_data->base,
	                                 clk_data);
	if (ret)
		goto unregister_muxs;

#if 1
	ret = faraday_clk_register_gates(gc1610_gate_clks,
	                                 ARRAY_SIZE(gc1610_gate_clks),
	                                 clk_data->base,
	                                 clk_data);
	if (ret)
		goto unregister_gates_busclk;
#endif
	ret = of_clk_add_provider(pdev->dev.of_node,
	                          of_clk_src_onecell_get, &clk_data->clk_data);
	if (ret)
		goto unregister_gates;

	platform_set_drvdata(pdev, clk_data);

	return 0;
#if 1
unregister_gates:
	faraday_clk_unregister_gates(gc1610_gate_clks,
				ARRAY_SIZE(gc1610_gate_clks),
				clk_data);
#endif
unregister_gates_busclk:
	faraday_clk_unregister_gates(gc1610_gate_busclks,
				ARRAY_SIZE(gc1610_gate_busclks),
				clk_data);
unregister_muxs:
	faraday_clk_unregister_muxs(gc1610_mux_clks,
				ARRAY_SIZE(gc1610_mux_clks),
				clk_data);
unregister_fixed_factors:
	faraday_clk_unregister_fixed_factors(gc1610_fixed_factor_clks,
				ARRAY_SIZE(gc1610_fixed_factor_clks),
				clk_data);
unregister_fixed_rates:
	faraday_clk_unregister_fixed_rates(gc1610_fixed_rate_clks,
				ARRAY_SIZE(gc1610_fixed_rate_clks),
				clk_data);
	return -EINVAL;
}

static int gc1610_clocks_remove(struct platform_device *pdev)
{
	struct faraday_clock_data *clk_data = platform_get_drvdata(pdev);

	of_clk_del_provider(pdev->dev.of_node);

	faraday_clk_unregister_gates(gc1610_gate_clks,
				ARRAY_SIZE(gc1610_gate_clks),
				clk_data);
	faraday_clk_unregister_gates(gc1610_gate_busclks,
				ARRAY_SIZE(gc1610_gate_busclks),
				clk_data);
	faraday_clk_unregister_muxs(gc1610_mux_clks,
				ARRAY_SIZE(gc1610_mux_clks),
				clk_data);
	faraday_clk_unregister_fixed_factors(gc1610_fixed_factor_clks,
				ARRAY_SIZE(gc1610_fixed_factor_clks),
				clk_data);
	faraday_clk_unregister_fixed_rates(gc1610_fixed_rate_clks,
				ARRAY_SIZE(gc1610_fixed_rate_clks),
				clk_data);

	return 0;
}

static const struct of_device_id gc1610_clocks_match_table[] = {
	{ .compatible = "faraday,gc1610-clk" },
	{ }
};
MODULE_DEVICE_TABLE(of, gc1610_clocks_match_table);

static struct platform_driver gc1610_clocks_driver = {
	.probe      = gc1610_clocks_probe,
	.remove     = gc1610_clocks_remove,
	.driver     = {
		.name   = "gc1610-clk",
		.owner  = THIS_MODULE,
		.of_match_table = gc1610_clocks_match_table,
	},
};

static int __init gc1610_clocks_init(void)
{
	return platform_driver_register(&gc1610_clocks_driver);
}
core_initcall(gc1610_clocks_init);

static void __exit gc1610_clocks_exit(void)
{
	platform_driver_unregister(&gc1610_clocks_driver);
}
module_exit(gc1610_clocks_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Fu-Tsung Hsu <mark_hs@faraday-tech.com>");
MODULE_DESCRIPTION("Faraday GC1610 Clock Driver");
