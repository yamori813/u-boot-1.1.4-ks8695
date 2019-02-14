/*
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
 * Misc functions
 */
#include <common.h>
#include <command.h>

#if (CONFIG_COMMANDS & CFG_CMD_MISC)

int do_sleep (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong start = get_timer(0);
	ulong delay;

	if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	delay = simple_strtoul(argv[1], NULL, 10) * CFG_HZ;

	while (get_timer(start) < delay) {
		if (ctrlc ()) {
			return (-1);
		}
		udelay (100);
	}

	return 0;
}

#if defined( CONFIG_DRIVER_KS8841 ) || defined( CONFIG_DRIVER_KS8842 )

int do_ks885x_hwgetcfg (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	hwinit();
	return 0;
}

int do_ks885x_regr(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong   BankNum;
	ulong   RegAddr;
    ulong   Width;

	if (argc != 4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	BankNum = simple_strtoul(argv[1], NULL, 16);
	RegAddr = simple_strtoul(argv[2], NULL, 16);
	Width = simple_strtoul(argv[3], NULL, 16);

    hwread ( BankNum, RegAddr, Width);

	return 0;
}

int do_ks885x_regw(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong   BankNum;
	ulong   RegAddr;
	ulong   RegData;
    ulong   Width;

	if (argc != 5) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	BankNum = simple_strtoul(argv[1], NULL, 16);
	RegAddr = simple_strtoul(argv[2], NULL, 16);
	RegData = simple_strtoul(argv[3], NULL, 16);
	Width = simple_strtoul(argv[4], NULL, 16);

    hwwrite ( BankNum, RegAddr, RegData, Width );

	return 0;
}

#endif /* #if defined( CONFIG_DRIVER_KS8841 ) || defined( CONFIG_DRIVER_KS8842 ) */

/* Implemented in $(CPU)/interrupts.c */
#if (CONFIG_COMMANDS & CFG_CMD_IRQ)
int do_irqinfo (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

U_BOOT_CMD(
	irqinfo,    1,    1,     do_irqinfo,
	"irqinfo - print information about IRQs\n",
	NULL
);
#endif  /* CONFIG_COMMANDS & CFG_CMD_IRQ */

U_BOOT_CMD(
	sleep ,    2,    2,     do_sleep,
	"sleep   - delay execution for some time\n",
	"N\n"
	"    - delay execution for N seconds (N is _decimal_ !!!)\n"
);

#if defined( CONFIG_DRIVER_KS8841 ) || defined( CONFIG_DRIVER_KS8842 )
U_BOOT_CMD(
	hwinit,  1,  1,     do_ks885x_hwgetcfg,
	"hwinit   - init ks8841 base address\n",
	NULL
);

U_BOOT_CMD(
	hwread ,   4,    4,     do_ks885x_regr,
	"hwread   - Read KS8851 register\n",
	NULL
);

U_BOOT_CMD(
	hwwrite ,   5,    5,     do_ks885x_regw,
	"hwwrite   - Modify KS8851 register\n",
	NULL
);
#endif /*#if defined( CONFIG_DRIVER_KS8841 ) || defined( CONFIG_DRIVER_KS8842 )*/

#endif	/* CFG_CMD_MISC */