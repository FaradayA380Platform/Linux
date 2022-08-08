/*
 *  arch/arm/mach-tc12ngrc/include/mach/hardware.h
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

#ifndef __MACH_HARDWARE_H
#define __MACH_HARDWARE_H

#include <asm/sizes.h>

/* Memory Mapped Addresses */

#define PLAT_SCU_BASE                   PLAT_FTSCU100_BASE

#define DEBUG_LL_FTUART010_PA_BASE      PLAT_FTUART010_BASE

/* AXI Devices */

/* AHB Devices */
#define PLAT_MPBLOCK_BASE               0x00200000
#define PLAT_FTSPI020_BASE              0x00500000
#define PLAT_FOTG210_BASE               0x00700000
#define PLAT_FOTG210_1_BASE             0x00600000
#define PLAT_MPBLOCK_1_BASE             0x00800000
#define PLAT_FTGMAC030_BASE             0x00C00000
#define PLAT_FTSDMC021_BASE             0x00D00000
#define PLAT_FTINTC020_BASE             0xFFFF0000
#define PLAT_FTDMAC020_BASE             0x00F00000
#define PLAT_MPBLOCK_2_BASE             0xFE000000

#define PLAT_MPBLOCK_VA_BASE            0xF0020000
#define PLAT_FTSPI020_VA_BASE           0xF0050000
#define PLAT_FOTG210_VA_BASE            0xF0070000
#define PLAT_FOTG210_1_VA_BASE          0xF0060000
#define PLAT_MPBLOCK_1_VA_BASE          0xF0080000
#define PLAT_FTGMAC030_VA_BASE          0xF00C0000
#define PLAT_FTSDMC021_VA_BASE          0xF00D0000
#define PLAT_FTINTC020_VA_BASE          0xF00E0000
#define PLAT_FTDMAC020_VA_BASE          0xF00F0000
#define PLAT_MPBLOCK_2_VA_BASE          0xFFE00000

/* APB Devices */
#define PLAT_EFUSE_CTRL_BASE            0xFFF00000
#define PLAT_EFUSE_CTRL_1_BASE          0xFFF10000
#define PLAT_FTUSART010_BASE            0xFFF20000
#define PLAT_FTWDT010_BASE              0xFFF30000
#define PLAT_FTGPIO010_BASE             0xFFF40000
#define PLAT_FTTMR010_BASE              0xFFF70000
#define PLAT_FTIIC010_BASE              0xFFF88000
#define PLAT_FTUART010_BASE             0xFFF8C000
#define PLAT_FTUART010_1_BASE           0xFFF90000
#define PLAT_FTSSP010_BASE              0xFFFA4000
#define PLAT_FTSSP010_1_BASE            0xFFFB8000
#define PLAT_FTADDC_BASE                0xFFFC0000
#define PLAT_FTSCU100_BASE              0xFFFD0000

#endif