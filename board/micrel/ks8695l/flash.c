/*
 * (C) Copyright 2005
 * Micrel, Inc. <www.micrel.com>
 *
 * (C) Copyright 2000
 * Marius Groeger <mgroeger@sysgo.de>
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 *
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * Flash Routines for Am29LV320DB flash chip based from board/oxa/flash.c.
 *
 *--------------------------------------------------------------------
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#if 0
#define DEBUG
#endif

#include <common.h>
#include <asm/arch/platform.h>

#define FLASH_MAN_HY (0xad << 16)
#define HY_ID_VL160B 0x2249

/* info for FLASH chips	*/
flash_info_t	flash_info[CFG_MAX_FLASH_BANKS];

/*-----------------------------------------------------------------------
 * Functions
 */

static ulong flash_get_size (vu_char *addr, flash_info_t *info);
static int write_word (flash_info_t *info, ulong dest, ushort data);

/*-----------------------------------------------------------------------
 */

unsigned long flash_init (void)
{
    unsigned long size;
    int i;
    unsigned int ulReg;

    ulReg = ks8695_read(KS8695_GPIO_MODE);
    ulReg |= 0x000000ff;
    ks8695_write(KS8695_GPIO_MODE, ulReg);
/*
    ulReg = ks8695_read(KS8695_GPIO_CTRL);
    ulReg |= 0x00000008; 
    ks8695_write(KS8695_GPIO_CTRL, ulReg);
*/
    ulReg = ks8695_read(KS8695_GPIO_DATA);
    ulReg &= 0xa0;
    ks8695_write(KS8695_GPIO_DATA, ulReg);

    /* Init: no FLASHes known */
    for (i=0; i<CFG_MAX_FLASH_BANKS; ++i) {
	flash_info[i].flash_id = FLASH_UNKNOWN;
    }

    size = flash_get_size((vu_char *)CFG_FLASH_BASE, &flash_info[0]);

    flash_protect (FLAG_PROTECT_SET,
		   CFG_FLASH_BASE,
		   CFG_FLASH_BASE + (U_BOOT_CODE_SIZE - 1),
		   &flash_info[0]);

#if (CFG_ENV_IS_IN_FLASH == 1) && defined(CFG_ENV_ADDR)
# ifndef  CFG_ENV_SIZE
#  define CFG_ENV_SIZE	CFG_ENV_SECT_SIZE
# endif
    flash_protect(FLAG_PROTECT_SET,
		  CFG_ENV_ADDR,
		  CFG_ENV_ADDR + CFG_ENV_SIZE - 1,
		  &flash_info[0]);
#endif

    return (size);
}

/*-----------------------------------------------------------------------
 */
void flash_print_info  (flash_info_t *info)
{
    int i;

    if (info->flash_id == FLASH_UNKNOWN) {
	printf ("missing or unknown FLASH type\n");
	return;
    }

    switch (info->flash_id & FLASH_VENDMASK) {
    case (AMD_MANUFACT & FLASH_VENDMASK):
	printf ("AMD ");
	break;
    case (MX_MANUFACT & FLASH_VENDMASK):
	printf ("MX ");
	break;
    case FLASH_MAN_STM:
	printf ("ST ");
	break;
    case FLASH_MAN_HY:
	printf ("Hynix ");
	break;
    default:
	printf ("Unknown Vendor ");
	break;
    }

    switch (info->flash_id & FLASH_TYPEMASK) {
    case (AMD_ID_LV320B & FLASH_TYPEMASK):
	printf ("Am29LV3200DB (32 Mbit, bottom boot block)\n");
	break;
    case (AMD_ID_LV800B & FLASH_TYPEMASK):
	printf ("LV800DB (8 Mbit, bottom boot block)\n");
	break;
    case FLASH_STM320DB:
	printf ("M29W320DB (32 Mbit)\n");
	break;
    case FLASH_STM800DB:
	printf ("M29W800DB (8 Mbit, bottom boot block)\n");
	break;
    case FLASH_STM800DT:
	printf ("M29W800DT (8 Mbit, top boot block)\n");
	break;
    case HY_ID_VL160B:
	printf ("HY29LV160 (16 Mbit, bottom boot block)\n");
	break;
    default:
	printf ("Unknown Chip Type\n");
	break;
    }

    printf ("  Size: %ld KB in %d Sectors\n",
	    info->size >> 10, info->sector_count);

    printf ("  Sector Start Addresses:");
    for (i=0; i<info->sector_count; ++i) {
	if ((i % 5) == 0)
	    printf ("\n   ");
	printf (" %08lX%s",
		info->start[i],
		info->protect[i] ? " (RO)" : "     "
		);
    }
    printf ("\n");
    return;
}

/*
 * The following code cannot be run from FLASH!
 */


static ulong flash_get_size (vu_char *addr, flash_info_t *info)
{
    short i;
    uchar vendor;
    ushort devid;
    ulong base = (ulong)addr;

    /* Write auto select command: read Manufacturer ID */
    addr[0x0AAA] = 0xAA;
    addr[0x0554] = 0x55;
    addr[0x0AAA] = 0x90;

    udelay(1000);

    vendor = addr[0];
    devid = *(( ushort* ) &addr[2]);

    if ( devid == ( HY_ID_VL160B & FLASH_TYPEMASK ) ) {
	info->flash_id     = vendor << 16 | devid;
	info->sector_count = 35;
	info->size         = 0x200000;
	info->start[0] = base;
	info->start[1] = base + 0x4000;
	info->start[2] = base + 0x4000 + 0x2000;
	info->start[3] = base + 0x4000 + 0x2000 + 0x2000;
	for (i = 4; i < info->sector_count; i++) {
	    info->start[i] = base + (i-3) * 0x10000;
	}
    } else if ( devid == ( AMD_ID_LV800B & FLASH_TYPEMASK ) ) {
	info->flash_id     = vendor << 16 | devid;
	info->sector_count = 19;
	info->size         = 0x100000;
	info->start[0] = base;
	info->start[1] = base + 0x4000;
	info->start[2] = base + 0x4000 + 0x2000;
	info->start[3] = base + 0x4000 + 0x2000 + 0x2000;
	for (i = 4; i < info->sector_count; i++) {
	    info->start[i] = base + (i-3) * 0x10000;
	}
    }
    else {
	return 0;
    }

    /* mark all sectors as unprotected */
    for (i = 0; i < info->sector_count; i++) {
	info->protect[i] = 0;
    }

    /* Issue the reset command */
    if (info->flash_id != FLASH_UNKNOWN) {
	addr[0] = 0xF0;	/* reset bank */
    }

    return (info->size);
}


/*-----------------------------------------------------------------------
 */

int flash_erase (flash_info_t *info, int s_first, int s_last)
{
    vu_char *addr = (vu_char *)(info->start[0]);
    int flag, prot, sect, l_sect;
    ulong start, now, last;
    int rc = 0;

    if ((s_first < 0) || (s_first > s_last)) {
	if (info->flash_id == FLASH_UNKNOWN) {
	    printf ("- missing\n");
	} else {
	    printf ("- no sectors to erase\n");
	}
	return 1;
    }

    prot = 0;
    for (sect = s_first; sect <= s_last; sect++) {
	if (info->protect[sect]) {
	    prot++;
	}
    }

    if (prot) {
	printf ("- Warning: %d protected sectors will not be erased!\n",
		prot);
    } else {
	printf ("\n");
    }

    l_sect = -1;

    /* Disable interrupts which might cause a timeout here */
    flag = disable_interrupts();

    addr[0x0AAA] = 0xAA;
    addr[0x0554] = 0x55;
    addr[0x0AAA] = 0x80;
    addr[0x0AAA] = 0xAA;
    addr[0x0554] = 0x55;

    /* wait at least 80us - let's wait 1 ms */
    udelay (1000);

    /* Start erase on unprotected sectors */
    for (sect = s_first; sect <= s_last; sect++) {
	if (info->protect[sect] == 0) {	/* not protected */
	    addr = (vu_char *)(info->start[sect]);
	    addr[0] = 0x30;
	    l_sect = sect;
	}
    }

    /* re-enable interrupts if necessary */
    if (flag)
	enable_interrupts();

    /* wait at least 80us - let's wait 1 ms */
    udelay (1000);

    /*
     * We wait for the last triggered sector
     */
    if (l_sect < 0)
	goto DONE;

    prot = s_last - s_first + 1;
    start = get_timer (0);
    last = 0;
    addr = (vu_char *)(info->start[l_sect]);
    serial_putc ('.');
    while ((addr[0] & 0x80) != 0x80) {
	if ((now = get_timer(start)) > CFG_FLASH_ERASE_TOUT * prot) {
	    printf ("Timeout\n");
	    rc = 1;
	    goto DONE;
	}
	/* show that we're waiting */
	if ((now - last) > 1000) {	/* every second */
	    serial_putc ('.');
	    last = now;
	}
    }
    printf (" done\n");

DONE:
    /* reset to read mode */
    addr = (volatile unsigned char *)info->start[0];
    addr[0] = 0xF0;	/* reset bank */

    return rc;
}


void inline spin_wheel (void)
{
	static int p = 0;
	static char w[] = "\\/-";

	printf ("\010%c", w[p]);
	(++p == 3) ? (p = 0) : 0;
}


/*-----------------------------------------------------------------------
 * Copy memory to flash, returns:
 * 0 - OK
 * 1 - write timeout
 * 2 - Flash not erased
 */

int write_buff (flash_info_t *info, uchar *src, ulong addr, ulong cnt)
{
    int rc;
    ushort* wsrc = ( ushort* ) src;
    int count = -1;

    cnt++;
    cnt >>= 1;
    while (cnt > 0) {
	if ((rc = write_word(info, addr, *wsrc)) != 0) {
	    return (rc);
	}
	addr++;
	addr++;
	wsrc++;
	cnt--;
	if (count++ > 0x2000) {
	    spin_wheel();
	    count = 0;
	}
    }
    if ( count >= 0 )
	printf("\010");

    return (0);
}

/*-----------------------------------------------------------------------
 * Write a byte to Flash, returns:
 * 0 - OK
 * 1 - write timeout
 * 2 - Flash not erased
 */
static int write_word (flash_info_t *info, ulong dest, ushort data)
{
    vu_char *addr = (vu_char *)(info->start[0]);
    ulong start;
    int flag;

    /* Check if Flash is (sufficiently) erased */
    if ((*((vu_short *)dest) & data) != data) {
	return (2);
    }

    /* Disable interrupts which might cause a timeout here */
    flag = disable_interrupts();

    addr[0x0AAA] = 0xAA;
    addr[0x0554] = 0x55;
    addr[0x0AAA] = 0xA0;

    *((vu_short *)dest) = data;

    /* re-enable interrupts if necessary */
    if (flag)
	enable_interrupts();

    /* data polling for D7 */
    start = get_timer (0);
    while ((*((vu_short *)dest) & 0x8080) != (data & 0x8080)) {
	if (get_timer(start) > CFG_FLASH_WRITE_TOUT) {
	    addr[0] = 0xF0;	/* reset bank */
	    return (1);
	}
    }
    return (0);
}

/*-----------------------------------------------------------------------
 */
