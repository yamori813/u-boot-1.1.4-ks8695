/*
 *  boot2L.S - codes to read u-boot from NAND to DDR during the initialization
 *
 *  Boot Stage 2 is running on temporary DDR location at 0x10000, just
 *  above the NAND default block size of 64K byte.
 *  Boot Stage 2 instruction is deposited by Boot Stage 1 code. At the
 *  end of Boot Stage 1, the execution is transferred to Boot Stage 2
 *
 *  Since Boot Stage 2 instruction is to be transfered to 0x10000 during the
 *  Boot Stage 1 execution running directly off NAND, no indirect
 *  reference can be used. The Boot Stage 2 instruction is built using
 *  combination of MOV and ADD insturction. The utility "code_convert" is
 *  used to automatically convert Boot Stage 2 instructions into codes to
 *  executed at Boot Stage 1.
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

/*
 *************************************************************************
 *
 * Upon entry,
 * r3 - ks8692 NAND base address
 * r4 - the first page to read from NAND
 * r5 - the number of blocks to read 
 * r6 - DDR address to write to
 * r7 - NAND offset register
 * r8 - New NAND offset
 *
 *************************************************************************
 */

	str	r8, [r7]
loop2:
	mov	r9, #2048/4		/* for large block NAND */
	add	r1, r3, #0x8000		/* NAND COMMAND register */
	add	r1, r1, #0x04		/* Index */
	str	r4, [r1]
	add	r1, r3, #0x8000		/* NAND COMMAND register */
	mov	r2, #0xee
	str	r2, [r1]
loop1:
	ldr	r0, [r3]		/* read data */
	str	r0, [r6], #4		/* write to DDR */
	subs	r9, r9, #1
	bne	-12			/* label loop1 */

	add	r4, r4, #4096		/* next page to open */
	subs	r5, r5, #1		/* count the block to read */
	bne	-52			/* label loop2 */

/*
 * U-boot entry point, TEXT_BASE, is defined in board/micrel/ks8692n/config.mk
 */
	mov	r0, #0x01000000		/* entry point of uboot */
	mov	pc, r0
