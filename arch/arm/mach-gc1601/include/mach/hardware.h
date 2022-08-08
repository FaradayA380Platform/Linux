/*
 *  arch/arm/mach-gc1601/include/mach/hardware.h
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

#ifndef __MACH_HARDWARE_H
#define __MACH_HARDWARE_H

#include <asm/sizes.h>

/* Memory Mapped Addresses */

#define PLAT_SCU_BASE                   PLAT_FTSCU100_BASE
#define PLAT_SCU_VA_BASE                PLAT_FTSCU100_VA_BASE

#define DEBUG_LL_FTUART010_PA_BASE      PLAT_FTUART010_BASE
#define DEBUG_LL_FTUART010_VA_BASE      PLAT_FTUART010_VA_BASE

/* AXI Devices */
#define PLAT_FTINTC030_BASE             0x96000000
#define PLAT_FTSPI020_BASE              0xA0000000

#define PLAT_FTINTC030_VA_BASE          0xF9600000
#define PLAT_FTSPI020_VA_BASE           0xFA000000

/* AHB Devices */

/* APB Devices */
#define PLAT_FTTMR010_BASE              0x90200000
#define PLAT_FTTMR010_1_BASE            0x90300000
#define PLAT_FTUART010_BASE             0x90400000
#define PLAT_FTIIC010_BASE              0x90600000
#define PLAT_FTIIC010_1_BASE            0x90700000
#define PLAT_FTWDT010_BASE              0x90800000
#define PLAT_FTGPIO010_BASE             0x90900000
#define PLAT_FTSCU100_BASE              0x90A00000

#define PLAT_FTTMR010_VA_BASE           0xF9020000
#define PLAT_FTTMR010_1_VA_BASE         0xF9030000
#define PLAT_FTUART010_VA_BASE          0xF9040000
#define PLAT_FTIIC010_VA_BASE           0xF9060000
#define PLAT_FTIIC010_1_VA_BASE         0xF9070000
#define PLAT_FTWDT010_VA_BASE           0xF9080000
#define PLAT_FTGPIO010_VA_BASE          0xF9090000
#define PLAT_FTSCU100_VA_BASE           0xF90A0000

#endif