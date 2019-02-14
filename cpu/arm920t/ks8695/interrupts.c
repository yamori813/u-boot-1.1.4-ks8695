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


#ifdef CONFIG_CENTAUR
#define	NR_IRQS		16
#define	PRINTF	printf
static struct _irq_handler
{
	void *arg;
	void (*handler)( void *arg);
} IRQ_HANDLER[NR_IRQS];


static unsigned int InstallIRQHandler(unsigned int routine, unsigned int *vector)
{
	unsigned int vec = 0, oldvec = 0;

	printf("handler=0x%x vector=0x%x\n", routine, vector);
	vec = ((routine - (unsigned int)vector - 0x8) >> 2 );
	if ( vec & 0xFF000000 )
	{
		printf("Installation of Handler failed\n");
		return -1;
	}

	vec = 0xEB000000 | vec;
	oldvec = *vector;
	*vector = vec;
	return (oldvec);
}

void do_irq (struct pt_regs *pt_regs);

void irq_install_handler(int irq, interrupt_handler_t *handler, void *arg)
{
	unsigned long uReg;
	static unsigned int *irqvec_start = (unsigned int*)0x18;
	//static unsigned int *irqvec_start = (unsigned int*)0x00;

	if (irq < 0 || irq >= NR_IRQS) {
		PRINTF("irq_install_handler: bad irq number %d\n", irq);
		return;
	}
//printf("irq_install_handler: irq=0x%x do_irq=0x%08x \n", irq, do_irq );	

	if (IRQ_HANDLER[irq].handler != NULL)
		PRINTF("IRQ_HANDLER: 0x%08lx replacing 0x%08lx\n",
			(ulong)handler, (ulong)IRQ_HANDLER[irq].handler);

	IRQ_HANDLER[irq].handler = handler;
	IRQ_HANDLER[irq].arg     = arg;

//	disable_interrupts();

	//1. Set Interrupt mode control as IRQ
	ks8695_write(KS8695_INT_CONTL,0);	//set to respond to IRQ, not FIQ

	if( (irq >= 2) && (irq <= 5) )
	{
		//2. Interrupt Priority register.
		//ks8695_write(KS8695_INT_EXT_PRIORITY,(0xf << irq-2) ); //priority = highest(0xf)

		//set GPIO mode(normal or special mode)
		uReg = ks8695_read(KS8695_GPIO_CTRL);
		uReg |= 0x8;		//EXT0 as External interrupt.
		ks8695_write(KS8695_GPIO_CTRL, uReg);

		uReg = ks8695_read(KS8695_GPIO_MODE);
		uReg = uReg & ~0x1;             //Set GPIO pins to be input.
		ks8695_write(KS8695_GPIO_MODE, uReg);
	}

	//3. Clear interrupt status.
	uReg = ks8695_read(KS8695_INT_STATUS);
	ks8695_write(KS8695_INT_STATUS, uReg);

	//4. Interrupt enable
	uReg = ks8695_read(KS8695_INT_ENABLE);
	uReg = (uReg & 0x4) | 0x4 ;            //IRQ2  
	ks8695_write(KS8695_INT_ENABLE, uReg);

//	wait_ms(1000);

	do
	{	
	static int enable=0;
/*
	wait_ms(1000);
printf("irq_install_handler: CTRL=0x%08x PRI=0x%08x STATUS=0x%08x ENABLE=0x%08x\n GPIO_CTRL=0x%08x GPIO_MODE=0x%08x\n MSTATUS=0x%x PFIQ=0x%x PIRQ=0x%x\n", 
	ks8695_read(KS8695_INT_CONTL), ks8695_read(KS8695_INT_EXT_PRIORITY),
	uReg=ks8695_read(KS8695_INT_STATUS), ks8695_read(KS8695_INT_ENABLE),
	ks8695_read(KS8695_GPIO_CTRL), ks8695_read(KS8695_GPIO_MODE),
	ks8695_read(KS8695_INT_MASK_STATUS),ks8695_read(KS8695_FIQ_PEND_PRIORITY),
	ks8695_read(KS8695_IRQ_PEND_PRIORITY) );

	//uReg = ks8695_read(KS8695_INT_STATUS);
	//uReg = 0;
	ks8695_write(KS8695_INT_STATUS, uReg);
*/
	if( !enable ) enable_interrupts();

	//enable=1;
	//wait_ms(100);
	//disp_offset("AFTER-INT");

	} while(0);

}

void irq_free_handler(int irq)
{
	unsigned long uReg;
	if (irq < 0 || irq >= NR_IRQS) {
		PRINTF("irq_free_handler: bad irq number %d\n", irq);
		return;
	}

	disable_interrupts();
	uReg = ks8695_read(KS8695_GPIO_MODE);
	uReg |= 0x0000000f;             //Set GPIO pins to be output.
	ks8695_write(KS8695_GPIO_MODE, uReg);

	uReg = ks8695_read(KS8695_INT_ENABLE);
	uReg &= ~(0x0000000f << 2);             
	ks8695_write(KS8695_INT_ENABLE, uReg);

	interrupt_init();
}

static void default_isr( void *data) 
{
	printf ("default_isr():  called for IRQ %d\n", (int)data);
	//printf ("+ \n");
	//wait_ms(3000);
}


void do_irq (struct pt_regs *pt_regs)
{
#ifdef CONFIG_USB_UHCI
	unsigned int pending;
	unsigned long uReg;
	extern int irqvec;
	static int irqcnt=0;
	unsigned int intstatus, check;

	uReg = ks8695_read(KS8695_INT_STATUS);

	if( (uReg >> 2 ) & 0x01 )
	{
		static unsigned long uRegMirror = 0;

		if( (uRegMirror >> 2 & 0xf) != ((uReg >> 2) & 0xf) )
		{
			//printf("*** do_irq:uReg=0x%x[old=0x%x] *****\n", uReg, uRegMirror);
			uRegMirror= uReg & 0x3f; 
		}

		if( (irqvec >= 0) )
		{
			int	irq=irqvec ;
			IRQ_HANDLER[irq].handler( IRQ_HANDLER[irq].arg );
		}
		else
			printf("Not registered IRQ\n");
	}
	else
	{
		if(irqcnt%30==1)
			printf("do_irq:: irqcnt=%d\n", irqcnt);
	}

	ks8695_write(KS8695_INT_STATUS, uReg);
#endif
}
#endif


int timer_inited;
ulong timer_ticks;

int interrupt_init (void)
{
#ifdef CONFIG_CENTAUR
	int i;

	/* install default interrupt handlers */
	for ( i = 0; i < NR_IRQS; i++) {
		IRQ_HANDLER[i].arg = (void *)i;
		IRQ_HANDLER[i].handler = default_isr;
	}

#else
	/* nothing happens here - we don't setup any IRQs */
#endif
	return (0);
}

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
	ks8695_write(KS8695_TIMER1, TIMER_COUNT);
	ks8695_write(KS8695_TIMER1_PCOUNT, TIMER_PULSE);
	ks8695_write(KS8695_TIMER_CTRL, 0x2);
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
	if (ks8695_read(KS8695_INT_STATUS) & KS8695_INTMASK_TIMERINT1) {
		/* Clear interrupt condition */
		ks8695_write(KS8695_INT_STATUS, KS8695_INTMASK_TIMERINT1);
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
	ulong end;

	if (!timer_inited)
		reset_timer();

	/* Only 1ms resolution :-( */
	end = usec / 1000;
	while (get_timer(start) < end)
		;
}

void reset_cpu (ulong ignored)
{
	ulong tc;

	/* Set timer0 to watchdog, and let it timeout */
	tc = ks8695_read(KS8695_TIMER_CTRL) & 0x2;
	ks8695_write(KS8695_TIMER_CTRL, tc);
	ks8695_write(KS8695_TIMER0, ((10 << 8) | 0xff));
	ks8695_write(KS8695_TIMER_CTRL, (tc | 0x1));

	/* Should only wait here till watchdog resets */
	for (;;)
		;
}


void raise ( int signal )
{
}
