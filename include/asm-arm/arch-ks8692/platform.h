/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#if 1	/* choose one */
#define KS8692_NAND_BOOT
#else
#define KS8692_NOR_BOOT
#endif

#include <asm/arch/ks8692Reg.h>

#ifndef __address_h
#define __address_h			1

#define KS8692_SDRAM_START	    0x00000000
#define KS8692_SDRAM_SIZE	    0x01000000
#define KS8692_MEM_SIZE		    KS8692_SDRAM_SIZE
#define KS8692_MEM_START	    KS8692_SDRAM_START

#define KS8692_IO_BASE		    0x1FFF0000
#define KS8692_IO_SIZE		    0x00010000
#define KS8692_FLASH_BASE	    0x18000000


#ifdef UBOOT_ORIG
#define KS8692_FLASH_START	    0x02800000
#else
#define KS8692_FLASH_START	    0x18000000
#endif
#define KS8692_FLASH_SIZE	    0x00400000

/*
 *  Timer definitions
 *
 *  Use timer 1 & 2
 *  (both run at 25MHz).
 *
 */
#define TICKS_PER_uSEC			25
#define mSEC_1				1000
#define mSEC_10				(mSEC_1 * 10)


/* SDRAM banks */
#define BOOT_START                  0x00400000   /*linux bootloader C code start address*/
#define DIAG_START                  0x00080000   /*diagnostic program C code start address*/
#define LEVEL1TABLE                 0x00040000   /*where the MMU table is stored */

#define SDRAM_NOP_COMD              0x30000
#define SDRAM_PRECHARGE_CMD         0x10000
#define SDRAM_REFRESH_TIMER         390                               
#define SDRAM_MODE_COMD             0x20033    
#define SDRAM_RASCAS                0x0000000A

#if !defined(CONFIG_ARCH_KS8692L)
#define FLASH_BANK_SIZE		    0x00800000  
#else
#define FLASH_BANK_SIZE		    0x00400000  
#endif

/* memory configuration */
#define FLASH_BANK		0
#define REMAPPED_FLASH_BANK	PHYS_FLASH_1

#define SDRAM_BANK_0		FLASH_BANK_SIZE
#define REMAPPED_SDRAM_BANK_0	PHYS_SDRAM_1

#define FLASH_BANK0_SIZE	0x00800000

#define REMAPPED_FLASH_BANK_1	(REMAPPED_FLASH_BANK + FLASH_BANK0_SIZE)

#define SDRAM_BANK0_SIZE	PHYS_SDRAM_1_SIZE
#define SDRAM_BANK1_SIZE	PHYS_SDRAM_2_SIZE

#define SDRAM_BANK_1		(SDRAM_BANK_0 + SDRAM_BANK0_SIZE)
#define SDRAM_BANK_END		(SDRAM_BANK_1 + SDRAM_BANK1_SIZE)

#define REMAPPED_SDRAM_BANK_1	(REMAPPED_SDRAM_BANK_0 + SDRAM_BANK0_SIZE)
#define REMAPPED_SDRAM_BANK_END	(REMAPPED_SDRAM_BANK_1 + SDRAM_BANK1_SIZE)
		
#define SDRAM_BANK_COLAB8	0x0
#define SDRAM_BANK_COLAB9	(0x1 << 8)
#define SDRAM_BANK_COLAB10	(0x2 << 8)
#define SDRAM_BANK_COLAB11	(0x3 << 8)
#define SDRAM_UNM_BANKS2	0x0
#define SDRAM_UNM_BANKS4	(0x1 << 3)
#define SDRAM_BANKS_DBW0	0x0
#define SDRAM_BANKS_DBW8	(0x1 << 1)
#define SDRAM_BANKS_DBW16	(0x2 << 1)
#define SDRAM_BANKS_DBW32	(0x3 << 1)

/* External I/O banks defintions */
#ifdef CONFIG_ARCH_KS8692MB
  #define EXT_IO_BANK_SIZE          0x00100000
  #define EXT_IO_BANK_0             0x03C00000
#elif defined( CONFIG_ARCH_KS8692PM )
  #define EXT_IO_BANK_SIZE          0x00400000
  #define EXT_IO_BANK_0             0x03200000
#else
  #define EXT_IO_BANK_SIZE          0x00200000
  #define EXT_IO_BANK_0             0x03400000
#endif

#define EXT_IO_BANK_1		    (EXT_IO_BANK_0 + EXT_IO_BANK_SIZE)  
#define EXT_IO_BANK_2		    (EXT_IO_BANK_1 + EXT_IO_BANK_SIZE)   
#define EXT_IOBANK_CLOCK0           0x0
#define EXT_IOBANK_CLOCK1           0x1
#define EXT_IOBANK_CLOCK2           0x2
#define EXT_IOBANK_CLOCK3           0x3
#define EXT_IOBANK_CLOCK4           0x4
#define EXT_IOBANK_CLOCK5           0x5
#define EXT_IOBANK_CLOCK6           0x6
#define EXT_IOBANK_CLOCK7           0x7
	
/* --- System memory locations */
#ifdef ROM_RAM_REMAP
 #define RAM_LIMIT                  REMAPPED_SDRAM_BANK_END  
#else
 #define RAM_LIMIT                  SDRAM_BANK_END  
#endif  

#define ABT_STACK                   RAM_LIMIT 
#define UNDEF_STACK                 ABT_STACK - 1024 
#define SVC_STACK                   UNDEF_STACK - 1024    
#define IRQ_STACK                   SVC_STACK - 2048      
#define FIQ_STACK                   IRQ_STACK - 4096
#define SYS_STACK                   FIQ_STACK - 4096
#define USR_STACK                   SYS_STACK - 4096
#define RAM_LIMIT_TMP               SDRAM_BANK_END - 2048
#define FLASH_ROM_START             REMAPPED_FLASH_BANK   
#define SDRAM_START                 SDRAM_BANK_0 

#if !defined(CONFIG_ARCH_KS8692L) && !defined( CONFIG_ARCH_KS8692V )
    #if defined( CONFIG_KS8692M )
	#define ROM_BANK_ACCESSTIME	ROM_BANK_ACCTM11_1
    #elif defined( CONFIG_ARCH_KS8692P )
	#define ROM_BANK_ACCESSTIME	ROM_BANK_ACCTM9_1
    #else
	#define ROM_BANK_ACCESSTIME	ROM_BANK_ACCTM9
    #endif
	#define SDRAM_BANKS_WIDTH	SDRAM_BANKS_DBW32
	#define REM_FLASH_REG1		0
#else
    #if defined( CONFIG_ARCH_KS8692V )
	#define ROM_BANK_ACCESSTIME	ROM_BANK_ACCTM11_1
    #else
	#define ROM_BANK_ACCESSTIME	ROM_BANK_ACCTM9
    #endif
    #if defined( CONFIG_KS8692V1 )
	#define SDRAM_BANKS_WIDTH	SDRAM_BANKS_DBW32
    #else
	#define SDRAM_BANKS_WIDTH	SDRAM_BANKS_DBW16
    #endif
	#define REM_FLASH_REG1		0
#endif

/*
 *  calculating all the memory bank configuration register value which used to take me a lot of 
 *  my time and often ends up with errors and now it will be done automatically.
 */	

#define FLASH_REG_VAL( start, size )  \
	((((start)+(size)-1)>>16)<<22)|(((start)>>16)<<12)|ROM_BANK_PMOD0|ROM_BANK_ACCESSTIME

#define SDRAM_REG_VAL( start, size )  \
	((((start)+(size)-1)>>16)<<22)|(((start)>>16)<<12)|SDRAM_UNM_BANKS4|SDRAM_BANKS_WIDTH

#define TMP_FLASH_REG0	FLASH_REG_VAL( FLASH_BANK, FLASH_BANK0_SIZE )
#define REM_FLASH_REG0	FLASH_REG_VAL( REMAPPED_FLASH_BANK, FLASH_BANK0_SIZE )

#define TMP_SDRAM_REG0	SDRAM_REG_VAL( SDRAM_BANK_0, SDRAM_BANK0_SIZE )
#define REM_SDRAM_REG0	SDRAM_REG_VAL( REMAPPED_SDRAM_BANK_0, SDRAM_BANK0_SIZE )


#if SDRAM_BANK1_SIZE == 0
 #define TMP_SDRAM_REG1    0
 #define SDRAM_REG1        0
 #define REM_SDRAM_REG1    0	
#else  
#define TMP_SDRAM_REG1	SDRAM_REG_VAL( SDRAM_BANK_1, SDRAM_BANK1_SIZE )
#define REM_SDRAM_REG1	SDRAM_REG_VAL( REMAPPED_SDRAM_BANK_1, SDRAM_BANK1_SIZE )

#endif

#if defined(CONFIG_ARCH_KS8692PM) || defined(CONFIG_ARCH_KS8692MB)
#ifdef REM_FLASH_REG1
#undef REM_FLASH_REG1
#endif
#define REM_FLASH_REG1	FLASH_REG_VAL( REMAPPED_FLASH_BANK_1, FLASH_BANK1_SIZE )
#endif

#define EXTIO_REG0         (((EXT_IO_BANK_0+EXT_IO_BANK_SIZE-1)>>16)<<22)|((EXT_IO_BANK_0>>16)<<12)|(EXT_IOBANK_CLOCK6<<9)
#define EXTIO_REG1         (((EXT_IO_BANK_1+EXT_IO_BANK_SIZE-1)>>16)<<22)|((EXT_IO_BANK_1>>16)<<12)|(EXT_IOBANK_CLOCK6<<9)
#define EXTIO_REG2         (((EXT_IO_BANK_2+EXT_IO_BANK_SIZE-1)>>16)<<22)|((EXT_IO_BANK_2>>16)<<12)|(EXT_IOBANK_CLOCK6<<9)

#endif
/* Registers for PCI Bridge (AHB Acess) */

#define REG_PCI_CONFID         0x2000
#define REG_PCI_CMD            0x2004
#define REG_PCI_REV            0x2008
#define REG_PCI_LTYTIMER       0x200C
#define REG_PCI_BASEMEM        0x2010
#define REG_PCI_BASEFST        0x2014
#define REG_PCI_BASELST        0x2028
#define REG_PCI_SYSID          0x202C
#define REG_PCI_INT            0x203C
#define REG_PCI_CNFADD         0x2100
#define REG_PCI_CNGDATA        0x2104
#define REG_PCI_BMODE          0x2200
#define REG_PCI_CONTROL        0x2204
#define REG_PCI_MEMBASE        0x2208
#define REG_PCI_MEMBCTRL       0x220C
#define REG_PCI_MBMARSK        0x2210
#define REG_PCI_MBTRANS        0x2214
#define REG_PCI_IOBASE         0x2218
#define REG_PCI_IOBCTRL        0x221C
#define REG_PCI_IOBMARSK       0x2220
#define REG_PCI_IOBTRANS       0x2224

/* Definition for PCI Bridge Registers */

#define PCI_CPU_MEM_BASE       0x10000000
#define PCI_CPU_IO_BASE        0x20000000
#define PCI_MEMBASE_MASK       0xF8000000
#define PCI_IOBASE_MASK        0xFF800000

#define PCI_MEM_PHYS           PCI_CPU_MEM_BASE
#define PCI_IO_PHYS            PCI_CPU_IO_BASE
#define PCI_MEM_SIZE           0x20000000   /* 512MB */

#define PCI_BMEM_PREFETCH      0x8
#define PCI_BMEM_PREFLMIT4     0x00000000
#define PCI_BMEM_PREFLMIT8     0x20000000
#define PCI_BMEM_PREFLMIT16    0x40000000
#define PCI_CONF_DISEXT        0x10000000

#define PCI_PCI_MODE           0x00000000
#define PCI_MINIPCI_MODE       0x20000000
#define PCI_CARDBUS_MODE       0x40000000
#define PCI_HOST_BRIDGE        0x80000000

#define PCI_LATENCY_TIMER_MASK 0x0000FF00           /* latency timer, 1 byte */
#define PCI_CACHE_LINESZ_MASK  0x000000FF           /* cache line size, 1 byte */

#define PCI_COMM_MEM               0x00000002          /* memory access enable */
#define PCI_COMM_MASTER            0x00000004          /* master enable */
#define PCI_COMM_PERRSP        0x00000040           /* parity error response */
#define PCI_COMM_SYSERREN          0x00000100           /* system error enable */
#define PCI_COMM_SETTING       (PCI_COMM_MEM | PCI_COMM_MASTER | PCI_COMM_PERRSP | PCI_COMM_SYSERREN)


#define PCI_ENABLE_ADDTRAN     0x80000000
#define PCI_NOEXIST            0xFFFF

/* Definition of IRQ vector */

#define	IRQ_SDIO        7
#define	IRQ_EHCI	    8
#define	IRQ_OHCI	    9
#define	IRQ_USBDEVICE	10
#define	IRQ_SPI         32+11
#define	IRQ_I2S_TX      32+10
#define	IRQ_I2S_RX      32+9
#define	IRQ_I2C         32+8


/*
 * TO access KS8692 device.
 */
#define	ks8692_read_byte(a)     *((volatile unchar *) (KS8692_IO_BASE + (a)))
#define	ks8692_read_word(a)     *((volatile ushort *) (KS8692_IO_BASE + (a)))
#define	ks8692_read_dword(a)    *((volatile ulong *)  (KS8692_IO_BASE + (a)))

#define	ks8692_write_byte(a,v)  *((volatile unchar *) (KS8692_IO_BASE + (a))) = (v)
#define	ks8692_write_word(a,v)  *((volatile ushort *) (KS8692_IO_BASE + (a))) = (v)
#define	ks8692_write_dword(a,v) *((volatile ulong *)  (KS8692_IO_BASE + (a))) = (v)

/*	END */
