/*
 * Faraday Clock Framework
 *
 * (C) Copyright 2021-2022 Faraday Technology
 * Bo-Cun Chen <bcchen@faraday-tech.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __DT_BINDINGS_CLOCK_GC1610_H__
#define __DT_BINDINGS_CLOCK_GC1610_H__

#define FARADAY_NR_CLKS          196

/* Fixed Rate */
#define GC1610_FIXED_XCLK25M            	1
#define GC1610_FIXED_TDMCLK24M        	2
#define GC1610_FIXED_ONUCLK155M        	3

/* PLL */
//#define GC1610_PLL0                11
//#define GC1610_PLL1                12
//#define GC1610_PLL2                13
//#define GC1610_PLL3                14

/* Fixed Factor */
#define GC1610_PLL0_CKOUT0B			11
#define GC1610_PLL0_CKOUT0B_DIV2		12
#define GC1610_PLL0_CKOUT0B_DIV4		13
#define GC1610_PLL1_U3_CLK2X			14
#define GC1610_PLL1_U3_CLK1X			15
#define GC1610_PLL1_U3_CLKLP			16
#define GC1610_PLL2_CKOUT1B			17
#define GC1610_PLL2_CKOUT0B			18
#define GC1610_PLL2_CKOUT0B_DIV3		19
#define GC1610_PLL2_CKOUT0B_DIV5		20
#define GC1610_PLL2_CKOUT0B_DIV10		21
#define GC1610_PLL2_CKOUT2B			22
#define GC1610_PLL2_CKOUT2B_DIV2		23
#define GC1610_PLL2_CKOUT2B_DIV4		24
#define GC1610_PLL2_CKOUT2B_DIV8		25
#define GC1610_PLL2_AXI4				26
#define GC1610_PLL2_AXI3				27
#define GC1610_PLL2_AHB				28
#define GC1610_PLL2_APB				29


#define GC1610_PLL3_FOUT3				30
#define GC1610_PLL3_FOUT3_DIV8		31
#define GC1610_PLL3_FOUT1				32
#define GC1610_PLL3_FOUT1_DIV3		33
#define GC1610_PLL3_FOUT1_DIV6		34
#define GC1610_PLL3_FOUT1_DIV12		35
#define GC1610_PLL4_FOUT1				36
#define GC1610_PLL5_FOUT1				37
#define GC1610_PLL5_FOUT3				38
#define GC1610_PLL5_FOUT2				39
#define GC1610_PLL6_FOUT1				40
#define GC1610_TDC_XCLK				41

/* Divider */

#define GC1610_TRACE_CLK           46
#define GC1610_GIC_ACLK            47
#define GC1610_LCD_SCACLK          48
#define GC1610_LCD_PIXCLK          49
#define GC1610_ADC_CLK_DIV         50
#define GC1610_GMAC0_CLK2P5M       51
#define GC1610_GMAC0_CLK25M        52
#define GC1610_GMAC0_CLK125M       53
#define GC1610_HB_CLK              54
#define GC1610_TDC_CLK_DIV         55
#define GC1610_GMAC1_CLK2P5M       56
#define GC1610_GMAC1_CLK25M        57
#define GC1610_GMAC1_CLK125M       58

/* MUX */
#define PLL0_CA53_CLKIN_STG1_MUX	60
#define PLL0_CA53_CLKIN_STG2_MUX	61
#define PLL2_AXI4_ACLK_MUX			62
#define PLL2_AXI3_ACLK_MUX			63
#define PLL2_ONU_PMU_ACLK_MUX		64
#define PLL2_PCLK_MUX				65
#define PLL3_TDM_MUX				66

/* GATED(BUS) */
#define GC1610_USB2_EN			81
#define GC1610_AES_EN			82
#define GC1610_NANDC_EN		83
#define GC1610_SPACC_EN		84
#define GC1610_TDM_EN			85


#define GC1610_EFUSE_PCLK_EN			90
#define GC1610_EMC0_PCLK_EN			91
#define GC1610_EMC1_PCLK_EN			92
#define GC1610_GPIO_PCLK_EN			93
#define GC1610_I2C0_PCLK_EN			94
#define GC1610_I2C1_PCLK_EN			95
#define GC1610_MDIO_PCLK_EN			96
#define GC1610_SSP_PCLK_EN				97
#define GC1610_TDC_PCLK_EN				98
#define GC1610_TMR0_PCLK_EN			99
#define GC1610_TMR1_PCLK_EN			100
#define GC1610_TMR2_PCLK_EN			101
#define GC1610_TMR3_PCLK_EN			102

#define GC1610_UART0_PCLK_EN			103
#define GC1610_UART1_PCLK_EN			104
#define GC1610_UART2_PCLK_EN			105
#define GC1610_UART3_PCLK_EN			106

#define GC1610_OTG210_PCLK_EN			107
#define GC1610_OTG330_PCLK_EN			108
#define GC1610_WDT0_PCLK_EN			109
//#define reserved			106
//#define GC1610_DDR_PCLK_EN			107
#define GC1610_ONU_PCLK_EN			110
#define GC1610_PCIE0_PCLK_EN			111
#define GC1610_PCIE1_PCLK_EN			110
//#define GC1610_SB_EXREG_PCLK_EN		111
//#define GC1610_X2X0_PCLK_EN			112
//#define GC1610_X2X1_PCLK_EN			113
//#define GC1610_SRDS0_SIGPCLK_EN		114
//#define GC1610_SRDS0_PCLK_EN			115
//#define GC1610_SRDS1_SIGPCLK_EN		116
//#define GC1610_SRDS1_PCLK_EN			117

	/* AXI Clock Control Register (SCU) */
#define GC1610_GIC_ACLK_EN				120
//#define GC1610_EMC0_ACLK_EN			121
//#define GC1610_EMC1_ACLK_EN			122
#define GC1610_SPI_ACLK_EN				123
#define GC1610_OTG330_ACLK_EN			124
#define GC1610_X2P1_ACLK_EN			125
#define GC1610_ONUPMU_ACLK_EN		126
#define GC1610_ONUWAC_ACLK_EN		127
#define GC1610_PCIE0_MST_ACLK_EN		128
#define GC1610_PCIE0_SLV_EN			129
#define GC1610_PCIE1_MST_ACLK_EN		130
#define GC1610_PCIE1_SLV_EN			131
#define GC1610_DDR_ACLK_EN			132




/* GATED(IP) */
#define GC1610_PLL0_CKOUT0B_EN        		141
#define GC1610_PLL0_CKOUT0B_DIV2_EN		142
#define GC1610_PLL0_CKOUT0B_DIV4_EN		143
#define GC1610_CA53_CLKIN_EN        			144
#define GC1610_CA53_PCLKIN_EN        			145
#define GC1610_PLL2_CKOUT1B_EN        		146
#define GC1610_PLL2_CKOUT0B_EN        		147
#define GC1610_PLL2_CKOUT2B_EN        		148
#define GC1610_AXI3_EN        					149
#define GC1610_ONUPMU_EN        				150
#define GC1610_HCLK_CK_EN        				151
#define GC1610_PCLK_CK_EN        				152
#define GC1610_PLL3_FOUT3_EN         			153
#define GC1610_PLL3_FOUT1_EN        			154

#endif	/* __DT_BINDINGS_CLOCK_GC1610_H__ */

