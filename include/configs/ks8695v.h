/*
 * (C) Copyright 2005
 * Micrel, Inc. <www.micrel.com>
 *
 * (C) Copyright 2005
 * Greg Ungerer <greg.ungerer@opengear.com>.
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * High Level Configuration Options
 * (easy to change)
 */
#define CONFIG_IDENT_STRING	"May 20, 2009"
#define CONFIG_KS8695	1		/* it is a KS8695 CPU */
#define CONFIG_CENTAUR  1		/* it is a Centaur board */

/* It is a KS8695P chip. */
#define CONFIG_KS8695P  1		/* it is KS8695P based */
#define CONFIG_ARCH_KS8695V	1
#define CONFIG_KS8695V1 1

#undef CONFIG_USE_IRQ			/* we don't need IRQ/FIQ stuff	*/

#define CONFIG_CMDLINE_TAG	 1	/* enable passing of ATAGs	*/
#define CONFIG_SETUP_MEMORY_TAGS 1
#define CONFIG_INITRD_TAG	 1

#define CONFIG_DRIVER_KS8695ETH		/* use KS8695 ethernet driver	*/

#define CFG_MALLOC_LEN		(CFG_ENV_SIZE + 128*1024)

/*
 * Size of malloc() pool
 */
#define CFG_GBL_DATA_SIZE	128	/* size in bytes reserved for initial data */

/*
 * Hardware drivers
 */

/*
 * select serial console configuration
 */
#define CFG_ENV_IS_IN_FLASH     1	/*save the uboot environment variables in flash */

#define	CONFIG_SERIAL1
#define CONFIG_CONS_INDEX	1
#define CONFIG_BAUDRATE		115200
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

#if 0
#define CONFIG_NETCONSOLE		/* include NetConsole support	*/
#endif
#if 0
#define CONFIG_NET_MULTI		/* required for netconsole      */
#endif

#define	CONFIG_COMMANDS		(CONFIG_CMD_DFL | CFG_CMD_DHCP)

/* this must be included AFTER the definition of CONFIG_COMMANDS (if any) */
#include <cmd_confdefs.h>

#define CONFIG_BOOTDELAY	3
#define CONFIG_BOOTARGS		"console=ttyAM0,115200"

/* uboot(0x20000) + env */
#define CONFIG_BOOTCOMMAND	"bootm 0x02030000"

/*
 * Miscellaneous configurable options
 */
#define CFG_LONGHELP				/* undef to save memory		*/
#define CFG_PROMPT		"boot > "	/* Monitor Command Prompt	*/
#define CFG_CBSIZE		256		/* Console I/O Buffer Size	*/
#define CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Print Buffer Size */
#define CFG_MAXARGS		16		/* max number of command args	*/
#define CFG_BARGSIZE		CFG_CBSIZE	/* Boot Argument Buffer Size	*/

#define CFG_MEMTEST_START	0x00800000	/* memtest works on	*/

#if defined( CONFIG_KS8695V1 )
#define CFG_MEMTEST_END		0x02000000	/* 32 MB in DRAM	*/

#elif defined( CONFIG_KS8695VS )
#define CFG_MEMTEST_END		0x00800000	/* 8 MB in DRAM	*/

#else
#define CFG_MEMTEST_END		0x01000000	/* 16 MB in DRAM	*/
#endif

#undef	CFG_CLKS_IN_HZ		/* everything, incl board info, in Hz */

#define CFG_LOAD_ADDR		0x00008000	/* default load address */

#define CFG_HZ			(1000)		/* 1ms resolution ticks */

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(128*1024)	/* regular stack */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ	(4*1024)	/* IRQ stack */
#define CONFIG_STACKSIZE_FIQ	(4*1024)	/* FIQ stack */
#endif

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#if defined( CONFIG_KS8695V1 )
#define CONFIG_NR_DRAM_BANKS	1	   /* we have 1 bank of DRAM */
#define PHYS_SDRAM_SIZE		0x02000000 /* 32 MB */

#elif defined( CONFIG_KS8695VS )
#define CONFIG_NR_DRAM_BANKS	1	   /* we have 1 bank of DRAM */
#define PHYS_SDRAM_SIZE		0x00800000 /* 8 MB */

#else
#define CONFIG_NR_DRAM_BANKS	2	   /* we have 2 banks of DRAM */
#define PHYS_SDRAM_SIZE		0x00800000 /* 8 MB */
#endif

#define PHYS_SDRAM_1		0x00000000 /* SDRAM Bank #1 */
#define PHYS_SDRAM_1_SIZE	PHYS_SDRAM_SIZE

#if ( CONFIG_NR_DRAM_BANKS > 1 )
#define PHYS_SDRAM_2		PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE
#define PHYS_SDRAM_2_SIZE	PHYS_SDRAM_SIZE

#else
#define PHYS_SDRAM_2		0
#define PHYS_SDRAM_2_SIZE	0
#endif

#define PHYS_FLASH_1		0x02000000 /* Flash Bank #1 */

#define PHYS_FLASH_SECT_SIZE    0x00010000 /* 64 KB sectors (x1) */

#define CFG_FLASH_BASE		PHYS_FLASH_1
#define	CFG_FLASH_SIZE		0x400000

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#define CFG_MAX_FLASH_BANKS	1	/* max number of flash banks */
#define	U_BOOT_CODE_SIZE	(PHYS_FLASH_SECT_SIZE * 2)

#define CFG_ENV_ADDR		(PHYS_FLASH_1 + U_BOOT_CODE_SIZE)
#define CFG_ENV_SECT_SIZE	PHYS_FLASH_SECT_SIZE	/* 64 KB sectors (x1) */
#define CFG_ENV_SIZE		PHYS_FLASH_SECT_SIZE	/* Total Size of Environment */

#define CFG_MAX_FLASH_SECT	(160)	/* max number of sectors on one chip */

/* timeout values are in ticks */
#define CFG_FLASH_WRITE_TOUT	(CFG_HZ/2)	/* Timeout for Flash Write */
#define CFG_FLASH_ERASE_TOUT	(CFG_HZ/2)	/* Timeout for Flash Erase */


#define	CONFIG_ETHADDR		00:10:A1:86:95:08
#define	CONFIG_ETH1ADDR		00:10:A1:86:95:18
#define CONFIG_HAS_ETH1
#define	CONFIG_IPADDR		192.168.1.200
#define	CONFIG_NETMASK		255.255.255.0
#define	CONFIG_SERVERIP 	192.168.1.11

#define	CONFIG_AUTOSCRIPT	1
#define	CONFIG_ENV_OVERWRITE

#define CFG_AUTOLOAD		"n"

#define	UBOOT_LOAD_RAMDISK	(TEXT_BASE - CFG_FLASH_SIZE)

#endif	/* __CONFIG_H */
