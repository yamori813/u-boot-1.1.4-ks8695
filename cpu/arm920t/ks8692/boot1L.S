/*
 *  boot1L.S - codes to initialize KS8692, DDR memory and
 *  establish boot2 code on DDR
 *
 *  Boot Stage 1 starts when system powers up. When system boots with
 *  NAND flash, the NAND is mapped to address zero to maximum 64K bytes.
 *  When program is executed directly off NAND, the program is limited
 *  to sequential execution until it jumps away and NAND is closed.
 *
 *  Boot Stage 1 code is to initialize DDR memory and establish
 *  Boot Stage 2 code in DDR memory. Since the program is to
 *  maintain sequential execution while achieve the functionality,
 *  cares are needed to prevent using relative reference instructions
 *  such as "ldr r1,=#0x12345678"
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>

#define UBOOT_LOCATION 0x00040000
#define UBOOT_PAGES 127

#ifdef CPU_INIT
	mrs	r0,cpsr
	bic	r0,r0,#0x1f
	orr	r0,r0,#0xd3
	msr	cpsr,r0
	/*
	 * flush v4 I/D caches
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0	/* flush v3/v4 cache */
	mcr	p15, 0, r0, c8, c7, 0	/* flush v4 TLB */

	/*
	 * disable MMU stuff and caches
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002300	@ clear bits 13, 9:8 (--V- --RS)
	bic	r0, r0, #0x00000087	@ clear bits 7, 2:0 (B--- -CAM)
	orr	r0, r0, #0x00000002	@ set bit 2 (A) Align
	orr	r0, r0, #0x00001000	@ set bit 12 (I) I-Cache
	mcr	p15, 0, r0, c1, c0, 0
#endif

#if defined( SAMSUNG_166_DDR )
#include "DDR-k4h51-166"
#elif defined( SAMSUNG_200_DDR )
#include "DDR-k4h51-200"
#elif defined( MICRON_166_DDR )
#include "DDR-mt46v-166"
#else
#include "DDR-mt46v-200"
#endif

	/* start to move Boot Stage 2 codes to DDR Memory area */
	mov	r1, #0x00010000		/* preboot.S assumes r1 is starting DDR address */

#include "prebootL.S"
	/* end of boot stage 2 preparation */

	/* preparing register for boot stage 2 */
	mov	r7, #0x1f000000
	add	r7, r7, #0x00ff0000
	add	r7, r7, #0x00000008	/* r7 - NAND offset register */
	mov	r8, #0x5f000000
	add	r8, r8, #0x00fe0000	/* r8 - New NAND address */

	mov	r3, #0x1f000000
	add	r3, r3, #0x00fe0000	/* r3 - NAND controller address */
	mov	r4, #UBOOT_LOCATION		/* r4 - first page to open, can be anywhere, must have 256K byte without bad block */
	mov	r5, #UBOOT_PAGES		/* r5 - number of pages to read - 127 pages */
	mov	r6, #0x01000000		/* uboot starting execution location */

	/* start booting */
	mov	r0, #0x00010000		/* preboot is loaded at 0x10000 */
	mov	pc, r0

