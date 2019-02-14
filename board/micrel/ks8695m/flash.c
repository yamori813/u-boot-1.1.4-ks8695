/*
 * (C) Copyright 2005
 * Micrel, Inc. <www.micrel.com>
 *
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

/*
 * This implementation is to support AMD29LV320DB, AM29LV640U, MX29LV320B, MX29LV640B
 *
 * 06/30/2006
 */

#include <common.h>
#include <linux/byteorder/swab.h>


flash_info_t flash_info[CFG_MAX_FLASH_BANKS];	/* info for FLASH chips */


#define mb() __asm__ __volatile__ ("" : : : "memory")

static inline void flash_reset ( vu_short* addr )
{
	*addr = 0xF0;	/* reset bank */
}

/*-----------------------------------------------------------------------
 * Functions
 */
static ulong flash_get_size (vu_short * addr, flash_info_t * info);
static int write_data (flash_info_t * info, ulong dest, unsigned short data);
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
			flash_get_size ((vu_short *) PHYS_FLASH_1, &flash_info[i]);
			flash_get_offsets (PHYS_FLASH_1, &flash_info[i]);
			break;
		case 1:
			flash_get_size ((vu_short *) PHYS_FLASH_2, &flash_info[i]);
			flash_get_offsets (PHYS_FLASH_2, &flash_info[i]);
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
	else if ((info->flash_id & FLASH_VENDMASK) == FLASH_MAN_MX) {
		if (((unsigned short)info->flash_id  == FLASH_MXLV640B) ||
		((unsigned short)info->flash_id == FLASH_MXLV320B)) {
			/* set sector offsets for bottom boot block type	*/
			info->start[0] = base + 0x00000000;
			info->start[1] = base + 0x00002000;
			info->start[2] = base + 0x00004000;
			info->start[3] = base + 0x00006000;
			info->start[4] = base + 0x00008000;
			info->start[5] = base + 0x0000a000;
			info->start[6] = base + 0x0000c000;
			info->start[7] = base + 0x0000e000;
			for (i = 8; i < info->sector_count; i++)
				info->start[i] = base + (i * 0x00010000) - 0x00070000;
		}
	}
	else if ((info->flash_id & FLASH_VENDMASK) == FLASH_MAN_AMD) {
	    if ((info->flash_id & FLASH_TYPEMASK) == FLASH_AM320B) {
		/* set sector offsets for bottom boot block type	*/
		info->start[0] = base + 0x00000000;
		info->start[1] = base + 0x00002000;
		info->start[2] = base + 0x00004000;
		info->start[3] = base + 0x00006000;
		info->start[4] = base + 0x00008000;
		info->start[5] = base + 0x0000a000;
		info->start[6] = base + 0x0000c000;
		info->start[7] = base + 0x0000e000;
		for (i = 8; i < info->sector_count; i++)
			info->start[i] = base + (i * 0x00010000) - 0x00070000;
		}
	    else if ((info->flash_id & FLASH_TYPEMASK) == FLASH_AMDL640) {
		for (i = 8; i < info->sector_count; i++)
			info->start[i] = base + i * 0x00010000;
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
	case FLASH_MXLV320B:	printf ("MXLV320B (32 Mbit, bottom boot)\n");
				break;
	case FLASH_MXLV640B:	printf ("MXLV640B (64 Mbit, bottom boot)\n");
				break;
	case FLASH_AM640U:	printf ("AM29LV640U (64 Mbit, uniform sector size)\n");
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
static ulong flash_get_size (vu_short * addr, flash_info_t * info)
{
	unsigned char value;

	/* Write auto select command: read Manufacturer ID */
	addr[0x555] = 0xAA;
	addr[0x2aa] = 0x55;
	addr[0x555] = 0x90;

	value = addr[0];

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
               debug ("defined Manufacturer ID = 0x%x\n", value);
		info->flash_id = FLASH_UNKNOWN;
		info->sector_count = 0;
		info->size = 0;
	}

	value = addr[1];	/* device ID            */

	debug (" Device: ID= 0x%x\n", value);

	switch (value&0xff) {

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

	case (unsigned char)MX_ID_LV320B:
		info->flash_id += FLASH_MXLV320B;
		info->sector_count = 71;
		info->size = 0x00400000;
		break;				/* => 4 MB		*/

	case (unsigned char)AMD_ID_LV320B:
		info->flash_id += FLASH_AM320B;
		info->sector_count = 71;
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
	case (unsigned char)MX_ID_LV640B:
		info->flash_id += FLASH_MXLV640B;
		info->sector_count = 135;
		info->size = 0x00800000;
		break;				/* => 8 MB		*/
	case (unsigned char)AMD_ID_LV640U:
		info->flash_id += FLASH_AM640U;
		info->sector_count = 128;
		info->size = 0x00800000;
		break;				/* => 8 MB		*/

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

	flash_reset( addr );

	return (info->size);
}


/*-----------------------------------------------------------------------
 */

int flash_erase (flash_info_t * info, int s_first, int s_last)
{
	vu_short* addr = ( vu_short* ) info->start[0];
	int flag, prot, sect, l_sect;
	ulong type;
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

	l_sect = -1;

	/* Disable interrupts which might cause a timeout here */
	flag = disable_interrupts ();

	addr[0x555] = 0xAA;
	addr[0x2AA] = 0x55;
	addr[0x555] = 0x80;

	addr[0x555] = 0xAA;
	addr[0x2AA] = 0x55;

	/* Start erase on unprotected sectors */
	for (sect = s_first; sect<=s_last; sect++) {
		if (info->protect[sect] == 0) {	/* not protected */
			addr = ( vu_short* ) info->start[sect];
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
	addr = ( vu_short* ) info->start[l_sect];

	putc ('.');
	while (( *addr & 0x80 ) != 0x80 ) {
		if ((now = get_timer(start)) > CFG_FLASH_ERASE_TOUT * prot ) {
			printf ("Timeout\n");
			flash_reset (( vu_short* ) info->start[0]);
			return 1;
		}

		/* show that we're waiting */
		if ((now - last) > 1000) {	/* every second */
			putc ('.');
			last = now;
		}
	}

DONE:
	/* reset to read mode */
	flash_reset (( vu_short* ) info->start[0]);

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
	unsigned short data;
	int count, i, l, rc, port_width;

	if (info->flash_id == FLASH_UNKNOWN)
		return 4;

	wp = addr;
	port_width = 2;

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
		if (count++ > 0x2000) {
			spin_wheel ();
			count = 0;
		}
	}
	if ( count >= 0 )
		printf("\010");

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
static int write_data (flash_info_t * info, ulong dest, unsigned short data)
{
	vu_short* addr = ( vu_short* ) info->start[0];
	vu_short* addr_dest = ( vu_short* ) dest;
	int flag;
	ulong start;
	unsigned short i;

/* convert to little indian */
	i = data >> 8;
	data = data << 8;
	data |= i & 0xff;
	/* Check if Flash is (sufficiently) erased */
	if (( *addr_dest & data ) != data ) {
		printf ("not erased at 0x%08lx (%x)\n", dest, *addr_dest);
		return (2);
	}

	/* write each byte out */
	flag = disable_interrupts();

	addr[0x555] = 0xAA;
	addr[0x2AA] = 0x55;
	addr[0x555] = 0xA0;

	*addr_dest = data;

	/* re-enable interrupts if necessary */
	if (flag)
		enable_interrupts();

	/* data polling for D7 */
	start = get_timer (0);
	while (( *addr_dest & 0x80 ) != ( data & 0x80 )) {
		if (get_timer(start) > CFG_FLASH_WRITE_TOUT) {
			flash_reset( addr );
			return (1);
		}
	}

	return (0);
}

void inline spin_wheel (void)
{
	static int p = 0;
	static char w[] = "\\/-";

	printf ("\010%c", w[p]);
	(++p == 3) ? (p = 0) : 0;
}
