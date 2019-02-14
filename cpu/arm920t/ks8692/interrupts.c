/*
 * (C) Copyright 2004-2005, Greg Ungerer <greg.ungerer@opengear.com>
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

#if (0)
#define  DEBUG
#endif

#ifdef  DEBUG
#define	DPRINT(a)	printf a;
#else   /* !DEBUG */
#define	DPRINT(a)
#endif  /* DEBUG */


#define	NR_IRQS		64
static struct _irq_handler
{
	void *arg;
	void (*handler)( void *arg);
} IRQ_HANDLER[NR_IRQS];

/*
 * Handy KS8692 register access functions.
 */
#define	ks8692_read(a)    *((volatile ulong *) (KS8692_IO_BASE + (a)))
#define	ks8692_write(a,v) *((volatile ulong *) (KS8692_IO_BASE + (a))) = (v)


/* IRQ and FIQ functions definition */
static void default_isr( void *);
void irq_install_handler(int, interrupt_handler_t *, void *);
void irq_free_handler(int);
int  interrupt_init (void);

void bad_mode (void);

/* IRQ functions definition */
void do_irq (struct pt_regs *);

/* FIQ functions definition */
void do_fiq (struct pt_regs *);


/*
 * set_gpio_to_int
 * Configure irq (2-4) from GPIO pin as external interrupt.
 *  triggerMode: 0-active low level;
 *               1-active high level;
 *               2-rising edge;
 *               3-rising edge;
 *               4-rising edge;
 */
void set_gpio_to_int(int irq, int triggerMode)
{
    unsigned long uReg;


    DPRINT(("%s: irq=%d, triggerMode=%d\n", __FUNCTION__, irq, triggerMode ));

    if( (irq >= 2) && (irq <= 5) )
    {
        uReg = ks8692_read(KS8692_GPIO_CTRL);
        uReg &= ~(0x0f << ((irq-2) << 2));	          /* clear trigger mode */
        
        /* set interrupt trigger mode */
        switch (triggerMode)
        {
             case 0:
                uReg |= (GPIO_INT_LOW << ((irq-2) << 2));/* EXT0-EXT4 for interrupt and level detection (active low) */
                break;
             case 1:
                uReg |= (GPIO_INT_HIGH << ((irq-2) << 2));	
                break;
             case 2:
                uReg |= (GPIO_INT_RISE_EDGE << ((irq-2) << 2));	
                break;
             case 3:
                uReg |= (GPIO_INT_FALL_EDGE << ((irq-2) << 2));	
                break;
             case 4:
                uReg |= (GPIO_INT_BOTH_EDGE << ((irq-2) << 2));	
                break;
        }

        /* set GPIO pin as external interrupt */
        uReg |= (0x08 << ((irq-2) << 2));	
        DPRINT(("%s: uReg=%08X:%08x\n", __FUNCTION__, KS8692_GPIO_CTRL, uReg ));
	    ks8692_write(KS8692_GPIO_CTRL, uReg);

        /* set GPIO pin as input pin */
        uReg = ks8692_read(KS8692_GPIO_MODE);
        uReg &= ~(0x01 << (irq-2));	          /* clear trigger mode */
        DPRINT(("%s: uReg=%08X:%08x\n", __FUNCTION__, KS8692_GPIO_MODE, uReg ));
        ks8692_write(KS8692_GPIO_MODE, uReg);
    }
    else
    {
        printf ("%s: ERROR! bad irq number %d to configure GPIO.\n", __FUNCTION__, (int)irq);
    }
}


static void default_isr( void *data) 
{
    printf ("%s: WARRING! called from IRQ %d without ISR, but interrupt is enabled.\n", __FUNCTION__, (int)data);
}

/*
 * do_irq:
 * There are totally 64 irq entries of vector table since 
 * the KS8692 provides 64 bits interrupt source, spread out 
 * in the two registers (KS8692_INT_ENABLE1, KS8692_INT_ENABLE2).
 * The KS8692_INT_ENABLE1 interrupt sources are in the lower part (0-31) of vector table.
 * The KS8692_INT_ENABLE2 interrupt sources are in the higher part (32-63) of vector table.
 * The default interrupt mode is IRQ (not FIQ).
 */
void do_irq (struct pt_regs *pt_regs)
{
#ifndef CONFIG_USE_IRQ
    printf ("interrupt request\n");
    show_regs (pt_regs);
    bad_mode ();

#else

    unsigned long intMask1, intMask2;
    int i;


    DPRINT(("%s: \n", __FUNCTION__ )); 

    /* read interrupt mask status */
    intMask1 = ks8692_read(KS8692_INT_MASK1_STATUS);
    intMask2 = ks8692_read(KS8692_INT_MASK2_STATUS);


    /* call interrupt handler if interrupt mask and interrupt status both are set */
    for (i=0; i<(NR_IRQS-32); i++)
    { 
        if ( (intMask1>>i) & 0x00000001 ) 
        {
            DPRINT(("%s: calling interrupt handle with this irq %d\n", __FUNCTION__, i ));
			IRQ_HANDLER[i].handler( IRQ_HANDLER[i].arg );
        }
    }

    for (i=0; i<(NR_IRQS-32); i++)
    {
        if ( (intMask2>>i) & 0x00000001 ) 
        {
            DPRINT(("%s: calling interrupt handle with this irq %d\n", __FUNCTION__, (i+32) ));
			IRQ_HANDLER[i+32].handler( IRQ_HANDLER[i+32].arg );
        }
    }

#endif  /* CONFIG_USE_IRQ */
}


/*
 * The FIQ mode hardware interrupt is working on KS8692. 
 * But since we don't have new function fiq_save_user_regs()\fiq_restore_user_regs() 
 * before and after call do_fiq(). After fiq ISR, system will crash.
 */

/*
 * do_fiq:
 * There are totally 64 fiq entries of vector table since 
 * the KS8692 provides 64 bits interrupt source, spread out 
 * in the two registers (KS8692_INT_ENABLE1, KS8692_INT_ENABLE2).
 * The KS8692_INT_ENABLE1 interrupt sources are in the lower part (0-31) of vector table.
 * The KS8692_INT_ENABLE2 interrupt sources are in the higher part (32-63) of vector table.
 * The default interrupt mode is IRQ (not FIQ). User has to enable spefic FIQ mode for 
 * that interrupt source. 
 */
void do_fiq (struct pt_regs *pt_regs)
{
#ifndef CONFIG_USE_IRQ
	printf ("fast interrupt request\n");
	show_regs (pt_regs);
	bad_mode ();
#else

    unsigned long intMask1, intMask2;
    int i;


    DPRINT(("%s: \n", __FUNCTION__ )); 

    /* read interrupt mask status */
    intMask1 = ks8692_read(KS8692_INT_MASK1_STATUS);
    intMask2 = ks8692_read(KS8692_INT_MASK2_STATUS);


    /* call interrupt handler if interrupt mask and interrupt status both are set */
    for (i=0; i<(NR_IRQS-32); i++)
    { 
        if ( (intMask1>>i) & 0x00000001 ) 
        {
            DPRINT(("%s: calling interrupt handle with this irq %d\n", __FUNCTION__, i ));
			IRQ_HANDLER[i].handler( IRQ_HANDLER[i].arg );
        }
    }

    for (i=0; i<(NR_IRQS-32); i++)
    {
        if ( (intMask2>>i) & 0x00000001 ) 
        {
            DPRINT(("%s: calling interrupt handle with this irq %d\n", __FUNCTION__, (i+32) ));
			IRQ_HANDLER[i+32].handler( IRQ_HANDLER[i+32].arg );
        }
    }

#endif  /* CONFIG_USE_IRQ */
}


/*
 * irq_install_handler:
 * Only install an interrupt handler from a specific interrupt sources.
 * The KS8692_INT_ENABLE1 interrupt sources are in the lower part of 
 * vector table (0~31). The KS8692_INT_ENABLE2 interrupt sources are 
 * in the higher part of vector table (32~63).
 */
void irq_install_handler(int irq, interrupt_handler_t *handler, void *arg)
{
    int enable;


    DPRINT(("%s: irq=%d, handler=0x%08x \n", __FUNCTION__, irq, (int)handler ));

    if (irq < 0 || irq >= NR_IRQS) {
        printf ("%s: ERROR! bad irq number %d.\n", __FUNCTION__, (int)irq);
        return;
    }
    if (handler == NULL) {
        printf ("%s: ERROR! bad handler %x.\n", __FUNCTION__, (int)handler);
        return;
    }

    enable = disable_interrupts ();

    IRQ_HANDLER[irq].handler = handler;
    IRQ_HANDLER[irq].arg     = arg;

    /* if (enable ) */
    {
        /* enable cpu interrupt handler */
        enable_interrupts();
        DPRINT(("%s: enable cpu interrupt handler. \n", __FUNCTION__ ));
    }
}


/*
 * irq_free_handler:
 * Free an interrupt handler.
 */
void irq_free_handler(int irq)
{
    int enable;


    DPRINT(("%s: irq=%d \n", __FUNCTION__, irq ));

    if (irq < 0 || irq >= NR_IRQS) {
        printf ("%s: ERROR! bad irq number %d.\n", __FUNCTION__, (int)irq);
        return;
    }

    enable = disable_interrupts ();

    /* Replace user specific ISR to default ISR */
    IRQ_HANDLER[irq].arg = (void *)irq;
    IRQ_HANDLER[irq].handler = default_isr;

    /* if (enable ) */
       /* enable cpu interrupt handler */
       enable_interrupts();
}


int interrupt_init (void)
{
#ifdef CONFIG_USE_IRQ
    static int initialized = 0;
    int i;

    DPRINT(("%s: \n", __FUNCTION__ ));

    /* only need to execute the following code once. */
    if (initialized == 0)
    {
        /* install default interrupt handlers */
        for ( i = 0; i < NR_IRQS; i++) {
            IRQ_HANDLER[i].arg = (void *)i;
            IRQ_HANDLER[i].handler = default_isr;
        }
        initialized ++;
    }
#else
    /* nothing happens here - we don't setup any IRQs */
#endif
    return (0);
}


int timer_inited;
ulong timer_ticks;


/*
 * Initial timer set constants. Nothing complicated, just set for a 1ms
 * tick.
 */
#define	TIMER_INTERVAL	(TICKS_PER_uSEC * mSEC_1)
#define	TIMER_COUNT	(TIMER_INTERVAL / 2)
#define	TIMER_PULSE	TIMER_COUNT

void reset_timer_masked(void)
{
	/* Set the hadware timer for 1ms */
	/* Need to disable timer first lest it may not work correctly. */
	ks8692_write(KS8692_TIMER_CTRL, 0x0);
	ks8692_write(KS8692_TIMER1, TIMER_COUNT);
	ks8692_write(KS8692_TIMER1_PCOUNT, TIMER_PULSE);
	ks8692_write(KS8692_TIMER_CTRL, 0x2);
	timer_ticks = 0;
	timer_inited++;
}

void reset_timer(void)
{
	reset_timer_masked();
}

ulong get_timer_masked(void)
{
	/* Check for timer wrap */
	if (ks8692_read(KS8692_INT_STATUS2) & INT_TIMER1) {
		/* Clear interrupt condition */
		ks8692_write(KS8692_INT_STATUS2, INT_TIMER1);
		timer_ticks++;
	}
	return timer_ticks;
}

ulong get_timer(ulong base)
{
       return (get_timer_masked() - base);
}

void set_timer(ulong t)
{
	timer_ticks = t;
}

void udelay(ulong usec)
{
	ulong start = get_timer_masked();
	/* Only 1ms resolution :-( */
	ulong end = usec / 1000;
	ulong tc;
	ulong interval = TICKS_PER_uSEC * ( usec % 1000 );
	ulong value = interval >> 1;
	ulong pcount = interval - value;

	if (!timer_inited)
		reset_timer();

	while (get_timer(start) < end)
		;

	if ( !interval )
		return;
	tc = ks8692_read( KS8692_TIMER_CTRL ) & 0x2;
	ks8692_write( KS8692_TIMER_CTRL, tc );
	ks8692_write( KS8692_TIMER0, value );
	ks8692_write( KS8692_TIMER0_PCOUNT, pcount );
	ks8692_write( KS8692_TIMER_CTRL, ( tc | 0x1 ));
	while ( !( ks8692_read( KS8692_INT_STATUS2 ) & INT_TIMER0 ) )
		;
	ks8692_write( KS8692_TIMER_CTRL, tc );
	ks8692_write( KS8692_INT_STATUS2, INT_TIMER0 );
}

void reset_cpu (ulong ignored)
{
	ulong tc;

	/* Set timer0 to watchdog, and let it timeout */
	tc = ks8692_read(KS8692_TIMER_CTRL) & 0x0;
	ks8692_write(KS8692_TIMER_CTRL, tc);
	ks8692_write(KS8692_TIMER0, ((10 << 8) | 0xff));
	ks8692_write(KS8692_TIMER0_PCOUNT, 0x10); /* active low period - 16 cycles */
	ks8692_write(KS8692_TIMER_CTRL, (tc | 0x1));

	/* Should only wait here till watchdog resets */
	for (;;)
		;
}


void raise ( int signal )
{
}
