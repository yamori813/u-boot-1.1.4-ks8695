/*
 * (C) Copyright 2005
 * Micrel, Inc. <www.micrel.com>
 *
 * (C) Copyright 2005
 * Greg Ungerer, OpenGear Inc, <greg.ungerer@opengear.com>
 *
 * (C) Copyright 2002
 * Kyle Harris, Nexus Technologies, Inc. kharris@nexus-tech.net
 *
 * (C) Copyright 2005
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
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
#include <asm/arch/platform.h>
#include <pci.h>

DECLARE_GLOBAL_DATA_PTR;

/* ------------------------------------------------------------------------- */


/*
 * Miscelaneous platform dependent initialisations
 */
int env_flash_cmdline (void)
{
	unsigned char *sp = (unsigned char *) 0x0201c020;
	unsigned char *ep;
	int len;

	/* Check for flash based kernel boot args to use as default */
	for (ep = sp, len = 0; ((len < 1024) && (*ep != 0)); ep++, len++)
		;

	if ((len > 0) && (len <1024))
		setenv("bootargs", (char*) sp);

	return 0;
}

int board_late_init (void)
{
	return 0;
}


int board_init (void)
{
	gd->bd->bi_arch_number = MACH_TYPE_KS8695;

	/* adress of boot parameters */
	gd->bd->bi_boot_params = 0x00000100;

	/* power down all but port 0 on the switch */
	ks8695_write(KS8695_SWITCH_LPPM12, 0x00000005);
	ks8695_write(KS8695_SWITCH_LPPM34, 0x00050005);

#if defined(CONFIG_PCI)
	/*
	 * Do pci configuration
	 */
	pci_init(); 
#endif

	return 0;
}

int dram_init (void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size  = PHYS_SDRAM_1_SIZE;

#if (CONFIG_NR_DRAM_BANKS > 1)
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size  = PHYS_SDRAM_2_SIZE;
#endif

	return (0);
}


/*
 * Initialize PCI Devices, report devices found.
 */

#ifdef	CONFIG_PCI
struct pci_controller hose;

extern void pci_ks869x_init(struct pci_controller *);

void pci_init_board(void)
{
	pci_ks869x_init(&hose);
}
#endif
