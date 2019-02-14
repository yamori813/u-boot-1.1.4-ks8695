/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * (C) Copyright 2003
 * Texas Instruments, <www.ti.com>
 * Kshitij Gupta <Kshitij@ti.com>
 *
 * (C) Copyright 2004
 * ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
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

DECLARE_GLOBAL_DATA_PTR;

void flash__init (void);
void ether__init (void);
void timer_init (void);
void peripheral_power_enable (void);
static ulong timestamp;
static ulong lastdec;

#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
	printf("Boot reached stage %d\n", progress);
}
#endif

#define COMP_MODE_ENABLE ((unsigned int)0x0000EAEF)

static inline void delay (unsigned long loops)
{
	__asm__ volatile ("1:\n"
		"subs %0, %1, #1\n"
		"bne 1b":"=r" (loops):"0" (loops));
}

/*
 * Miscellaneous platform dependent initialisations
 */

int board_init (void)
{
	/*
	 * set clock frequency:
	 *	VERSATILE_REFCLK is 32KHz
	 *	VERSATILE_TIMCLK is 1MHz
	 */
	*(volatile unsigned int *)(VERSATILE_SCTL_BASE) |=
	  ((VERSATILE_TIMCLK << VERSATILE_TIMER1_EnSel) | (VERSATILE_TIMCLK << VERSATILE_TIMER2_EnSel) |
	   (VERSATILE_TIMCLK << VERSATILE_TIMER3_EnSel) | (VERSATILE_TIMCLK << VERSATILE_TIMER4_EnSel));

#ifdef CONFIG_ARCH_VERSATILE_AB
	gd->bd->bi_arch_number = MACH_TYPE_VERSATILE_AB;
#else
 	gd->bd->bi_arch_number = MACH_TYPE_VERSATILE_PB;
#endif

	/* adress of boot parameters */
	gd->bd->bi_boot_params = 0x00000100;

	gd->flags = 0;

	icache_enable ();

	flash__init ();
	ether__init ();
	timer_init();
	return 0;
}


int misc_init_r (void)
{
	setenv("verify", "n");
	return (0);
}

/******************************
 Routine:
 Description:
******************************/
void flash__init (void)
{
	/* Use the sytem control register to allow writing to flash */
	unsigned int tmp = *(unsigned int *)(VERSATILE_FLASHCTRL);
	tmp |= VERSATILE_FLASHPROG_FLVPPEN;
	*(unsigned int *)(VERSATILE_FLASHCTRL) = tmp;
}
/*************************************************************
 Routine:ether__init
 Description: take the Ethernet controller out of reset and wait
	      for the EEPROM load to complete.
*************************************************************/
void ether__init (void)
{
}

/******************************
 Routine:
 Description:
******************************/
int dram_init (void)
{
	return 0;
}

void timer_init(void){
	/*
	 * Set clock frequency in the system controller:
	 *	VERSATILE_REFCLK is 32KHz
	 *	VERSATILE_TIMCLK is 1MHz
	 */
	*(volatile unsigned int *)(VERSATILE_SCTL_BASE) |=
		((VERSATILE_TIMCLK << VERSATILE_TIMER1_EnSel) | (VERSATILE_TIMCLK << VERSATILE_TIMER2_EnSel) |
		 (VERSATILE_TIMCLK << VERSATILE_TIMER3_EnSel) | (VERSATILE_TIMCLK << VERSATILE_TIMER4_EnSel));
	/*
	 * Now setup timer0
	 */
	*(volatile ulong *)(CFG_TIMERBASE + 0) = CFG_TIMER_RELOAD;	/* TimerLoad */
	*(volatile ulong *)(CFG_TIMERBASE + 4) = CFG_TIMER_RELOAD;	/* TimerValue */
	*(volatile ulong *)(CFG_TIMERBASE + 8) |= 0x82;			/* Enabled,
									 * free running,
									 * no interrupt,
									 * 32-bit,
									 * wrapping
									 */
	reset_timer_masked();
}
int interrupt_init (void){
	return 0;
}

/*
 * Write the system control status register to cause reset
 */
void reset_cpu(ulong addr){
	*(volatile unsigned int *)(VERSATILE_SCTL_BASE + SP810_OS_SCSYSSTAT) = 0x00000000;
}
/* delay x useconds AND perserve advance timstamp value */
/* ASSUMES timer is ticking at 1 msec			*/
void udelay (unsigned long usec)
{
	ulong tmo, tmp;

	tmo = usec/1000;

	tmp = get_timer (0);		/* get current timestamp */

	if( (tmo + tmp + 1) < tmp )	/* if setting this forward will roll time stamp */
		reset_timer_masked ();	/* reset "advancing" timestamp to 0, set lastdec value */
	else
		tmo += tmp;		/* else, set advancing stamp wake up time */

	while (get_timer_masked () < tmo)/* loop till event */
		/*NOP*/;
}

ulong get_timer (ulong base)
{
	return get_timer_masked () - base;
}

void reset_timer_masked (void)
{
	/* reset time */
	lastdec = READ_TIMER/1000;	/* capure current decrementer value time */
	timestamp = 0;			/* start "advancing" time stamp from 0 */
}

/* ASSUMES 1MHz timer */
ulong get_timer_masked (void)
{
	ulong now = READ_TIMER/1000;	/* current tick value @ 1 tick per msec */

	if (lastdec >= now) {		/* normal mode (non roll) */
		/* normal mode */
		timestamp += lastdec - now; /* move stamp forward with absolute diff ticks */
	} else {
		/* we have overflow of the count down timer */
		/* nts = ts + ld + (TLV - now)
		 * ts=old stamp, ld=time that passed before passing through -1
		 * (TLV-now) amount of time after passing though -1
		 * nts = new "advancing time stamp"...it could also roll and cause problems.
		 */
		timestamp += lastdec + TIMER_LOAD_VAL - now;
	}
	lastdec = now;

	return timestamp;
}


