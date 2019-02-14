/*
 * (C) Copyright 2005
 * Greg Ungerer, OpenGear Inc, greg.ungerer@opengear.com
 *
 * (C) Copyright 2001
 * Kyle Harris, Nexus Technologies, Inc. kharris@nexus-tech.net
 *
 * (C) Copyright 2001
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

#if !defined( CFG_NO_FLASH ) && !defined( CFG_FLASH_CFI_DRIVER )
#include <linux/byteorder/swab.h>

static void out8(ulong addr, uchar value)
{
        *(uchar *)addr=value;
}

static uchar in8(ulong addr)
{
        return *(uchar *)addr;
}

#define iobarrier_rw()  {}

static void flash_reset (ulong addr)
{
        out8(addr, 0xF0);       /* reset bank */
        iobarrier_rw();
}

flash_info_t flash_info[CFG_MAX_FLASH_BANKS];	/* info for FLASH chips */

#define mb() __asm__ __volatile__ ("" : : : "memory")

/*-----------------------------------------------------------------------
 * Functions
 */
static ulong flash_get_size (unsigned char * addr, flash_info_t * info);
static int write_data (flash_info_t * info, ulong dest, unsigned char data);
static void flash_get_offsets (ulong base, flash_info_t * info);
void inline spin_wheel (void);

/*-----------------------------------------------------------------------
 */

unsigned long flash_init (void)
{
	int i;
	ulong size = 0;

	for (i = 0; i < CFG_MAX_FLASH_BANKS; i++) {
		switch (i) {
		case 0:
			flash_get_size ((unsigned char *) PHYS_FLASH_1, &flash_info[i]);
			flash_get_offsets (PHYS_FLASH_1, &flash_info[i]);
			break;
		case 1:
			/* ignore for now */
			flash_info[i].flash_id = FLASH_UNKNOWN;
			break;
		default:
			panic ("configured too many flash banks!\n");
			break;
		}
		size += flash_info[i].size;
	}

	/* Protect monitor and environment sectors
	 */

	flash_protect (FLAG_PROTECT_SET,
		       CFG_FLASH_BASE,
		       CFG_FLASH_BASE + (U_BOOT_CODE_SIZE + CFG_ENV_SECT_SIZE - 1),
		       &flash_info[0]);

	return size;
}

/*-----------------------------------------------------------------------
 */
static void flash_get_offsets (ulong base, flash_info_t * info)
{
	int i;

	if (info->flash_id == FLASH_UNKNOWN)
		return;

	/* set up sector start address table */
	if ((info->flash_id & FLASH_VENDMASK) == FLASH_MAN_INTEL) {
		for (i = 0; i < info->sector_count; i++) {
			info->start[i] = base + (i * PHYS_FLASH_SECT_SIZE);
			info->protect[i] = 0;
		}
	}

	else if (((info->flash_id & FLASH_VENDMASK) == FLASH_MAN_SST) ||
		(info->flash_id  == FLASH_AM040) ||
		(info->flash_id == FLASH_AMDLV033C) ||
		(info->flash_id == FLASH_AMDLV065D)) {
		ulong sectsize = info->size / info->sector_count;
		for (i = 0; i < info->sector_count; i++)
			info->start[i] = base + (i * sectsize);
	} else {
	    if (info->flash_id & FLASH_BTYPE) {
		/* set sector offsets for bottom boot block type	*/
		info->start[0] = base + 0x00000000;
		info->start[1] = base + 0x00004000;
		info->start[2] = base + 0x00006000;
		info->start[3] = base + 0x00008000;
		for (i = 4; i < info->sector_count; i++) {
			info->start[i] = base + (i * 0x00010000) - 0x00030000;
		}
	    } else {
		/* set sector offsets for top boot block type		*/
		i = info->sector_count - 1;
		info->start[i--] = base + info->size - 0x00004000;
		info->start[i--] = base + info->size - 0x00006000;
		info->start[i--] = base + info->size - 0x00008000;
		for (; i >= 0; i--) {
			info->start[i] = base + i * 0x00010000;
		}
	    }
	}

}

/*-----------------------------------------------------------------------
 */
void flash_print_info (flash_info_t * info)
{
	int i;

	if (info->flash_id == FLASH_UNKNOWN) {
		printf ("missing or unknown FLASH type\n");
		return;
	}

	switch (info->flash_id & FLASH_VENDMASK) {
	case FLASH_MAN_AMD:	printf ("AMD ");		break;
	case FLASH_MAN_INTEL:	printf ("INTEL ");		break;
	case FLASH_MAN_FUJ:	printf ("FUJITSU ");		break;
	case FLASH_MAN_SST:	printf ("SST ");		break;
	case FLASH_MAN_MX:	printf ("MXIC ");		break;

	default:		printf ("Unknown Vendor ");	break;
	}

	switch (info->flash_id & FLASH_TYPEMASK) {
	case FLASH_AM040:	printf ("AM29F040 (512 Kbit, uniform sector size)\n");
				break;
	case FLASH_AM400B:	printf ("AM29LV400B (4 Mbit, bottom boot sect)\n");
				break;
	case FLASH_AM400T:	printf ("AM29LV400T (4 Mbit, top boot sector)\n");
				break;
	case FLASH_AM800B:	printf ("AM29LV800B (8 Mbit, bottom boot sect)\n");
				break;
	case FLASH_AM800T:	printf ("AM29LV800T (8 Mbit, top boot sector)\n");
				break;
	case FLASH_AM160B:	printf ("AM29LV160B (16 Mbit, bottom boot sect)\n");
				break;
	case FLASH_AM160T:	printf ("AM29LV160T (16 Mbit, top boot sector)\n");
				break;
	case FLASH_AM320B:	printf ("AM29LV320B (32 Mbit, bottom boot sect)\n");
				break;
	case FLASH_AM320T:	printf ("AM29LV320T (32 Mbit, top boot sector)\n");
				break;
	case FLASH_AMDLV033C:	printf ("AM29LV033C (32 Mbit, uniform sector size)\n");
				break;
	case FLASH_AMDLV065D:	printf ("AM29LV065D (64 Mbit, uniform sector size)\n");
				break;
	case FLASH_SST800A:	printf ("SST39LF/VF800 (8 Mbit, uniform sector size)\n");
				break;
	case FLASH_SST160A:	printf ("SST39LF/VF160 (16 Mbit, uniform sector size)\n");
				break;
	case FLASH_SST040:	printf ("SST39LF/VF040 (4 Mbit, uniform sector size)\n");
				break;
	default:		printf ("Unknown Chip Type\n");
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
static ulong flash_get_size (unsigned char * addr, flash_info_t * info)
{
	volatile unsigned char value,value1,value2,value3;

        out8(addr + 0x0555, 0xAA);
        iobarrier_rw();
        out8(addr + 0x02AA, 0x55);
        iobarrier_rw();
        out8(addr + 0x0555, 0x90);
        iobarrier_rw();

        value = in8(addr);
        iobarrier_rw();

	debug ("Flash start address =0x%x\n", addr);

        switch (value) {

        case (INTEL_MANUFACT & 0xFFFF):
                debug ("Manufacturer: Intel (not supported yet)\n");
                info->flash_id = FLASH_MAN_INTEL;
                break;

        case (AMD_MANUFACT & 0xFFFF):
                debug ("Manufacturer: AMD (Spansion)\n");
                info->flash_id = FLASH_MAN_AMD;
                break;

        case (SST_MANUFACT & 0xFFFF):
                debug ("Manufacturer: SST\n");
                info->flash_id = FLASH_MAN_SST;
                break;

        case (FUJ_MANUFACT & 0xFFFF):
                debug ("Manufacturer: FUJITSU (Spansion)\n");
                info->flash_id = FLASH_MAN_FUJ;
                break;

        case (MX_MANUFACT & 0xFFFF):
                debug ("Manufacturer: MX\n");
                info->flash_id = FLASH_MAN_MX;
                break;
	default:
                debug ("Undefined Manufacturer ID\n");
		info->flash_id = FLASH_UNKNOWN;
		info->sector_count = 0;
		info->size = 0;
		addr[0] = 0xFF;	/* restore read mode */
		return (0);	/* no or unknown flash  */
	}

#ifdef DEL
	mb ();
	value = addr[1];	/* device ID            */
#endif
        value = in8(addr+1);
        iobarrier_rw();

	debug (" Device: ID= 0x%x\n", value);

	switch (value) {

	case (unsigned char)INTEL_ID_28F640J3A:
		info->flash_id += FLASH_28F640J3A;
		info->sector_count = 64;
		info->size = 0x00800000;
		break;		/* => 8 MB     */

	case (unsigned char)INTEL_ID_28F128J3A:
		info->flash_id += FLASH_28F128J3A;
		info->sector_count = 128;
		info->size = 0x01000000;
		break;		/* => 16 MB     */

	case (unsigned char)AMD_ID_F040B:
		info->flash_id += FLASH_AM040;
		info->sector_count = 8;
		info->size = 0x0080000; /* => 512 ko */
		break;

	case (unsigned char)AMD_ID_LV400T:
		info->flash_id += FLASH_AM400T;
		info->sector_count = 11;
		info->size = 0x00080000;
		break;				/* => 0.5 MB		*/

	case (unsigned char)AMD_ID_LV400B:
		info->flash_id += FLASH_AM400B;
		info->sector_count = 11;
		info->size = 0x00080000;
		break;				/* => 0.5 MB		*/

	case (unsigned char)AMD_ID_LV800T:
		info->flash_id += FLASH_AM800T;
		info->sector_count = 19;
		info->size = 0x00100000;
		break;				/* => 1 MB		*/

	case (unsigned char)AMD_ID_LV800B:
		info->flash_id += FLASH_AM800B;
		info->sector_count = 19;
		info->size = 0x00100000;
		break;				/* => 1 MB		*/

	case (unsigned char)AMD_ID_LV160T:
		info->flash_id += FLASH_AM160T;
		info->sector_count = 35;
		info->size = 0x00200000;
		break;				/* => 2 MB		*/

	case (unsigned char)AMD_ID_LV160B:
		info->flash_id += FLASH_AM160B;
		info->sector_count = 35;
		info->size = 0x00200000;
		break;				/* => 2 MB		*/

	case (unsigned char)AMD_ID_LV033C:
		info->flash_id += FLASH_AMDLV033C;
		info->sector_count = 64;
		info->size = 0x00400000;
		break;				/* => 4 MB		*/

	case (unsigned char)AMD_ID_LV065D:
		info->flash_id += FLASH_AMDLV065D;
		info->sector_count = 128;
		info->size = 0x00800000;
		break;				/* => 8 MB		*/

	case (unsigned char)AMD_ID_LV320T:
		info->flash_id += FLASH_AM320T;
		info->sector_count = 67;
		info->size = 0x00400000;
		break;				/* => 4 MB		*/

	case (unsigned char)AMD_ID_LV320B:
		info->flash_id += FLASH_AM320B;
		info->sector_count = 67;
		info->size = 0x00400000;
		break;				/* => 4 MB		*/

	case (unsigned char)SST_ID_xF800A:
		info->flash_id += FLASH_SST800A;
		info->sector_count = 16;
		info->size = 0x00100000;
		break;				/* => 1 MB		*/

	case (unsigned char)SST_ID_xF160A:
		info->flash_id += FLASH_SST160A;
		info->sector_count = 32;
		info->size = 0x00200000;
		break;				/* => 2 MB		*/
	case (unsigned char)SST_ID_xF040:
		info->flash_id += FLASH_SST040;
		info->sector_count = 128;
		info->size = 0x00080000;
		break;				/* => 512KB		*/

	default:
                debug ("Undefined Device ID\n");
		info->flash_id = FLASH_UNKNOWN;
		break;
	}

	if (info->sector_count > CFG_MAX_FLASH_SECT) {
		printf ("** ERROR: sector count %d > max (%d) **\n",
			info->sector_count, CFG_MAX_FLASH_SECT);
		info->sector_count = CFG_MAX_FLASH_SECT;
	}
#ifdef UBOOT_ORIG
	addr[0] = 0xFF;	/* restore read mode */
#endif

	flash_reset(addr);

	return (info->size);
}


/*-----------------------------------------------------------------------
 */

int flash_erase (flash_info_t * info, int s_first, int s_last)
{
	volatile u32 addr = info->start[0];
	int flag, prot, sect, l_sect;
	ulong type;
	int rcode = 0;
	ulong start, now, last;

	if ((s_first < 0) || (s_first > s_last)) {
		if (info->flash_id == FLASH_UNKNOWN) {
			printf ("- missing\n");
		} else {
			printf ("- no sectors to erase\n");
		}
		return 1;
	}

	type = (info->flash_id & FLASH_VENDMASK);
	if (info->flash_id == FLASH_UNKNOWN) {
		printf ("Can't erase unknown flash type %08lx - aborted\n",
			info->flash_id);
		return 1;
	}

	prot = 0;
	for (sect = s_first; sect <= s_last; ++sect) {
		if (info->protect[sect]) {
			prot++;
		}
	}

	if (prot)
		printf ("- Warning: %d protected sectors will not be erased!\n", prot);
	else
		printf ("\n");

	/* Disable interrupts which might cause a timeout here */
	flag = disable_interrupts ();

	/* Start erase  n unprotected sectors */
	out8(addr + 0x555, 0xAA);
        iobarrier_rw();
        out8(addr + 0x2AA, 0x55);
        iobarrier_rw();
        out8(addr + 0x555, 0x80);
        iobarrier_rw();
        out8(addr + 0x555, 0xAA);
        iobarrier_rw();
        out8(addr + 0x2AA, 0x55);
        iobarrier_rw();
#ifdef DEL
	for (sect = s_first; sect <= s_last; sect++) {
		if (info->protect[sect] == 0) {	/* not protected */
			volatile unsigned char *addr;
			unsigned char status;

			printf ("Erasing sector %2d ... ", sect);

			/* arm simple, non interrupt dependent timer */
			reset_timer_masked ();

			addr = (volatile unsigned char *) (info->start[sect]);
			*addr = 0x50;	/* clear status register */
			*addr = 0x20;	/* erase setup */
			*addr = 0xD0;	/* erase confirm */

			while (((status = *addr) & 0x80) != 0x80) {
				if (get_timer_masked () >
				    CFG_FLASH_ERASE_TOUT) {
					printf ("Timeout\n");
					*addr = 0xB0;	/* suspend erase */
					*addr = 0xFF;	/* reset to read mode */
					rcode = 1;
					break;
				}
			}

			*addr = 0x50;	/* clear status register cmd */
			*addr = 0xFF;	/* resest to read mode */

			printf (" done\n");
		}
	}
	return rcode;
#endif

	/* Start erase on unprotected sectors */
	for (sect = s_first; sect<=s_last; sect++) {
		if (info->protect[sect] == 0) {	/* not protected */
			addr = info->start[sect];
			out8(addr, 0x30);
			iobarrier_rw();
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

	start = get_timer (0);
	last  = start;
	addr = info->start[l_sect];

//	debug("Start erase timeout: %d\n", CFG_FLASH_ERASE_TOUT);

	while ((in8(addr) & 0x80) != 0x80) {
		if ((now = get_timer(start)) > CFG_FLASH_ERASE_TOUT) {
			printf ("Timeout\n");
			flash_reset (info->start[0]);
			return 1;
		}
		/* show that we're waiting */
		if ((now - last) > 1000) {	/* every second */
			putc ('.');
			last = now;
		}
		iobarrier_rw();
	}

DONE:
	/* reset to read mode */
	flash_reset (info->start[0]);

	printf (" done\n");
	return 0;
}

/*-----------------------------------------------------------------------
 * Copy memory to flash, returns:
 * 0 - OK
 * 1 - write timeout
 * 2 - Flash not erased
 * 4 - Flash not identified
 */

int write_buff (flash_info_t * info, uchar * src, ulong addr, ulong cnt)
{
	ulong cp, wp;
	unsigned char data;
	int count, i, l, rc, port_width;

	if (info->flash_id == FLASH_UNKNOWN)
		return 4;

	wp = addr;
	port_width = 1;

	/*
	 * handle unaligned start bytes
	 */
	if ((l = addr - wp) != 0) {
		data = 0;
		for (i = 0, cp = wp; i < l; ++i, ++cp) {
			data = (data << 8) | (*(uchar *) cp);
		}
		for (; i < port_width && cnt > 0; ++i) {
			data = (data << 8) | *src++;
			--cnt;
			++cp;
		}
		for (; cnt == 0 && i < port_width; ++i, ++cp) {
			data = (data << 8) | (*(uchar *) cp);
		}

		if ((rc = write_data (info, wp, data)) != 0) {
			return (rc);
		}
		wp += port_width;
	}

	/*
	 * handle word aligned part
	 */
	count = 0;
	while (cnt >= port_width) {
		data = 0;
		for (i = 0; i < port_width; ++i) {
			data = (data << 8) | *src++;
		}
		if ((rc = write_data (info, wp, data)) != 0) {
			return (rc);
		}
		wp += port_width;
		cnt -= port_width;
		if (count++ > 0x800) {
			spin_wheel ();
			count = 0;
		}
	}

	if (cnt == 0) {
		return (0);
	}

	/*
	 * handle unaligned tail bytes
	 */
	data = 0;
	for (i = 0, cp = wp; i < port_width && cnt > 0; ++i, ++cp) {
		data = (data << 8) | *src++;
		--cnt;
	}
	for (; i < port_width; ++i, ++cp) {
		data = (data << 8) | (*(uchar *) cp);
	}

	return (write_data (info, wp, data));
}

/*-----------------------------------------------------------------------
 * Write a word or halfword to Flash, returns:
 * 0 - OK
 * 1 - write timeout
 * 2 - Flash not erased
 */
static int write_data (flash_info_t * info, ulong dest, unsigned char data)
{
	volatile unsigned char *addr = (volatile unsigned char *) dest;
	ulong status, start;
	int flag, i=0;

	/* Check if Flash is (sufficiently) erased */
	if ((*addr & data) != data) {
		printf ("not erased at %08lx (%lx)\n", (ulong) addr,
			(ulong) * addr);
		return (2);
	}

	/* write each byte out */
//	for (i = 0; i < 4; i++) 
	{
		char *data_ch = (char *)&data;
		int flag = disable_interrupts();

		out8(addr + 0x555, 0xAA);
		iobarrier_rw();
		out8(addr + 0x2AA, 0x55);
		iobarrier_rw();
		out8(addr + 0x555, 0xA0);
		iobarrier_rw();
		out8(dest+i, data_ch[i]);
		iobarrier_rw();

		/* re-enable interrupts if necessary */
		if (flag)
			enable_interrupts();

		/* data polling for D7 */
		start = get_timer (0);
		while ((in8(dest+i) & 0x80) != (data_ch[i] & 0x80)) {
			if (get_timer(start) > CFG_FLASH_WRITE_TOUT) {
				flash_reset (addr);
				return (1);
			}
			iobarrier_rw();
		}
	}

	flash_reset (addr);
	return (0);
}

void inline spin_wheel (void)
{
	static int p = 0;
	static char w[] = "\\/-";

	printf ("\010%c", w[p]);
	(++p == 3) ? (p = 0) : 0;
}
#endif
