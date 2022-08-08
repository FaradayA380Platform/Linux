/*
 * arch/arm/mach-gc1601/include/mach/irqs.h
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

#ifndef __MACH_IRQS_H
#define __MACH_IRQS_H

#define IRQ_START                   (32)

#define NR_IRQS                     (80)

/*
 * Interrupt numbers of Hierarchical Architecture
 */
#define PLAT_SYSC_IRQ               (IRQ_START + 0)
#define PLAT_FTGPIO010_IRQ          (IRQ_START + 1)
#define PLAT_FTIIC010_IRQ           (IRQ_START + 2)
#define PLAT_FTIIC010_1_IRQ         (IRQ_START + 3)
#define PLAT_FTTMR010_T1_IRQ        (IRQ_START + 4)
#define PLAT_FTTMR010_T2_IRQ        (IRQ_START + 5)
#define PLAT_FTTMR010_T3_IRQ        (IRQ_START + 6)
#define PLAT_FTTMR010_IRQ           (IRQ_START + 7)
#define PLAT_FTTMR010_1_T1_IRQ      (IRQ_START + 8)
#define PLAT_FTTMR010_1_T2_IRQ      (IRQ_START + 9)
#define PLAT_FTTMR010_1_T3_IRQ      (IRQ_START + 10)
#define PLAT_FTTMR010_1_IRQ         (IRQ_START + 11)
#define PLAT_FTUART010_IRQ          (IRQ_START + 12)
#define PLAT_FTWDT010_IRQ           (IRQ_START + 13)
#define PLAT_FTSPI020_IRQ           (IRQ_START + 14)

#endif    /* __MACH_IRQS_H */
