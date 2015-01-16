/*
 *  arch/arm/mach-sun7i/include/mach/memory.h
 *
 * Copyright (c) Allwinner.  All rights reserved.
 * Benn Huang (benn@allwinnertech.com)
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
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#define PLAT_PHYS_OFFSET                UL(0x40000000)
#define PLAT_MEM_SIZE                   SZ_512M

#define SYS_CONFIG_MEMBASE             (PLAT_PHYS_OFFSET + SZ_32M + SZ_16M) /* +48M */
#define SYS_CONFIG_MEMSIZE             (SZ_64K) /* 64K */



#define SUPER_STANDBY_SIZE             0x00020000 /* SZ_128K */
#define SUPER_STANDBY_BASE             (0x52000000) /* NOTICE: this addr can not be change */

/*
 * memory reserved areas.
 */
#define SW_VE_MEM_BASE                 (PLAT_PHYS_OFFSET + SZ_64M)

#define HW_RESERVED_MEM_BASE           (SW_VE_MEM_BASE)    
#define HW_RESERVED_MEM_SIZE_1G        (0x08000000 + 0x01400000)   		/* 116M(DE+VE(CSI)+FB) */
#define HW_RESERVED_MEM_SIZE_512M      (0x04000000 + 0x02200000)   		/* 80M(DE+VE(CSI)+FB) */

#define SW_GPU_MEM_BASE                (HW_RESERVED_MEM_BASE + HW_RESERVED_MEM_SIZE_1G)
#define SW_GPU_MEM_SIZE_1G             0x04000000 /* SZ_64M */
#define SW_GPU_MEM_SIZE_512M           0x04000000 /* SZ_64M */

#define SW_G2D_MEM_BASE                (SW_GPU_MEM_BASE + SW_GPU_MEM_SIZE_1G)
#define SW_G2D_MEM_SIZE                0 

#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
#define ION_CARVEOUT_MEM_BASE          (SUPER_STANDBY_BASE + SUPER_STANDBY_SIZE) /* +332M */
#define ION_CARVEOUT_MEM_SIZE          (CONFIG_ION_SUNXI_CARVEOUT_SIZE * SZ_1M)  /* in Mbytes */
#endif

#endif
