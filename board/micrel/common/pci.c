/*
 * Copyright (C) 2006 Micrel Inc.
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
 * PCI Configuration space access support for Micrel KS869x  
 * (ARM922T code) PCI Host Bridge
 */
#include <common.h>
#include <asm/arch/platform.h>
#include <asm/io.h>
#include <pci.h>


#if defined(CONFIG_PCI)

#if defined( CONFIG_ARCH_KS8692P )

/*
 * TO access KS8692 device.
 */
#define	ks_read(a)                ks8692_read_dword(a)    
#define	ks_write(a,v)             ks8692_write_dword(a,v) 

#else

/*
 * TO access KS8695 device.
 */
#define	ks_read(a)                ks8695_read(a)	        
#define	ks_write(a,v)             ks8695_write(a,v)       

#endif


void out_8 (volatile unsigned *addr, char val)
{
	*addr = val;
}

void out_le16 (volatile unsigned *addr, unsigned short val)
{
	*addr = cpu_to_le16 (val);
}

void out_le32 (volatile unsigned *addr, unsigned int val)
{
	*addr = cpu_to_le32 (val);
}

unsigned char in_8 (volatile unsigned *addr)
{
	unsigned char val;

	val = *addr;
	return val;
}

unsigned short in_le16 (volatile unsigned *addr)
{
	unsigned short val;

	val = *addr;
	val = le16_to_cpu (val);
	return val;
}

unsigned in_le32 (volatile unsigned *addr)
{
	unsigned int val;

	val = *addr;
	val = le32_to_cpu (val);
	return val;
}

/*============================================================================
*
* PCIMAKECONFADDR -
*
* This macro will generate a address to access PCI device for KS869x PBCA register  
* from bus number, device number, function number, and register offset.
*
* Type 0:
*
*  3 3|3 3 2 2|2 2 2 2|2 2 2 2|1 1 1 1|1 1 1 1|1 1
*  3 2|1 0 9 8|7 6 5 4|3 2 1 0|9 8 7 6|5 4 3 2|1 0 9 8|7 6 5 4|3 2 1 0
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
* | | |D|D|D|D|D|D|D|D|D|D|D|D|D|D|D|D|D|D|D|D|D|F|F|F|R|R|R|R|R|R|0|0|
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*	31:11	Device select bit.
* 	10:8	Function number
* 	 7:2	Register number
*
* Type 1:
*
*  3 3|3 3 2 2|2 2 2 2|2 2 2 2|1 1 1 1|1 1 1 1|1 1
*  3 2|1 0 9 8|7 6 5 4|3 2 1 0|9 8 7 6|5 4 3 2|1 0 9 8|7 6 5 4|3 2 1 0
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
* | | | | | | | | | | |B|B|B|B|B|B|B|B|D|D|D|D|D|F|F|F|R|R|R|R|R|R|0|1|
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*	31:24	reserved
*	23:16	bus number (8 bits = 128 possible buses)
*	15:11	Device number (5 bits)
*	10:8	function number
*	 7:2	register number
*
* RETURNS: configuration address on the PCI bus
============================================================================*/

#define PCIMAKECONFADDR( busNum, deviceNum, funNum, regNum, dwPBCA )        \
{                                                                           \
    if (busNum == 0 )                                                       \
       *(unsigned long *)dwPBCA = (unsigned long)(0x80000000|((deviceNum & 0x1F) << 11 )|((funNum & 0x07) << 8)|regNum) ; \
    else                                                                     \
       *(unsigned long *)dwPBCA = (unsigned long)(((busNum & 0xFF) << 16)|((deviceNum & 0x1F)<< 11)|((funNum & 0x07) << 8)|regNum|0x1); \
}


/*============================================================================
 *
 * routine:	Pci_CfgRead_dword()
 *
 * parameters:	bus = bus #
 *              device = device #
 *              function = function #
 *              offset = offset into PCI config space for current BAR
 *              data   = value to be written
 *
 * description:	This routine read a dword value from the given PCI config register.
 *
 *
 * returns:	data - value read.
 ============================================================================*/

unsigned long Pci_CfgRead_dword
(
    unsigned int busNum, 
    unsigned int deviceNum, 
    unsigned int funNum, 
    unsigned int regNum
)
{
    unsigned long  address;
    unsigned long  data;

    /* Generate the address value for the content of PBCA to access PCI device configuration space */
    PCIMAKECONFADDR (busNum, deviceNum, funNum, regNum, &address );


    /* Now, we have valid params, go read the config space data */
    ks_write(REG_PCI_CNFADD, address);
    data = ks_read(REG_PCI_CNGDATA);

    return (data);
}


/*============================================================================
 *
 * routine:	Pci_CfgWrite_dword()
 *
 * parameters:	bus = bus #
 *              device = device #
 *              function = function #
 *              offset = offset into PCI config space for current BAR
 *              data   = value to be written
 *              pStatus = SUCCESS/FAIL flag
 *
 * description:	This routines write a dword value to the given PCI config register.
 *
 *
 * returns:	none
 *============================================================================*/

void Pci_CfgWrite_dword
(   unsigned int busNum, 
    unsigned int deviceNum, 
    unsigned int funNum, 
    unsigned int regNum, 
    unsigned int data
)
{
    unsigned int  address;

    /* Generate the address value for the content of PBCA to access PCI device configuration space */
    PCIMAKECONFADDR (busNum, deviceNum, funNum, regNum, &address );

    /* Now, we have valid params, go write the data to the config space */
    ks_write(REG_PCI_CNFADD, address);
    ks_write(REG_PCI_CNGDATA, data);

    return;
}

int ks869x_pci_CfgRead_byte
(
    struct pci_controller *hose, 
    pci_dev_t dev,
    int offset, 
    u8 *val
)
{
    unsigned long  address;
    unsigned long  cfgData;
    unsigned char  byteLane;

    /* Generate the address value for the content of PBCA to access PCI device configuration space */
    address = (unsigned long) (dev | offset | 0x80000000);

    /* Now, we have valid params, go read the config space data */
    ks_write ((u16)hose->cfg_addr, address); 
    cfgData = ks_read((u16)hose->cfg_data);

    byteLane = (u8)(offset & 0x03);

    *val = (u8)(cfgData >> (8 * byteLane));

    return 0;
}

int ks869x_pci_CfgRead_word
(
    struct pci_controller *hose, 
    pci_dev_t dev,
    int offset, 
    u16 *val
)
{
    unsigned long  address;
    unsigned long  cfgData;
    unsigned char  byteLane;


    /* Generate the address value for the content of PBCA to access PCI device configuration space */
    address = (unsigned long) (dev | offset | 0x80000000);

    /* Now, we have valid params, go read the config space data */
    ks_write ((u16)hose->cfg_addr, address); 
    cfgData = ks_read((u16)hose->cfg_data);

    byteLane = (u8)(offset & 0x03);

    *val = (u16)(cfgData >> (8 * byteLane));

    return 0;
}

int ks869x_pci_CfgRead_dword
(
    struct pci_controller *hose, 
    pci_dev_t dev,
    int offset, 
    u32 *val
)
{
    unsigned long  address;

	
    /* Generate the address value for the content of PBCA to access PCI device configuration space */
    address = (unsigned long) (dev | offset | 0x80000000);

    /* Now, we have valid params, go read the config space data */
    ks_write ((u16)(hose->cfg_addr), address); 

    *val = ks_read((u16)hose->cfg_data);

    return 0;
}


int ks869x_pci_CfgWrite_byte
(
    struct pci_controller *hose, 
    pci_dev_t dev,
    int offset, 
    u8 val
)
{
    unsigned long  address;


    /* Generate the address value for the content of PBCA to access PCI device configuration space */
    address = (unsigned long) (dev | offset | 0x80000000);

    /* Now, we have valid params, go read the config space data */
    ks_write ((u16)hose->cfg_addr, address); 
    ks_write ((u16)hose->cfg_data, val); 

    return 0;
}

int ks869x_pci_CfgWrite_word
(
    struct pci_controller *hose, 
    pci_dev_t dev,
    int offset, 
    u16 val
)
{
    unsigned long  address;

	/* printf ("ks869x_pci_CfgWrite_word() is called.\n"); */

    /* Generate the address value for the content of PBCA to access PCI device configuration space */
    address = (unsigned long) (dev | offset | 0x80000000);

    /* Now, we have valid params, go read the config space data */
    ks_write ((u16)hose->cfg_addr, address); 
    ks_write ((u16)hose->cfg_data, val); 

    return 0;
}

int ks869x_pci_CfgWrite_dword
(
    struct pci_controller *hose, 
    pci_dev_t dev,
    int offset, 
    u32 val
)
{
    unsigned long  address;

	/* printf ("ks869x_pci_CfgWrite_dword() is called.\n"); */

    /* Generate the address value for the content of PBCA to access PCI device configuration space */
    address = (unsigned long) (dev | offset | 0x80000000);

    /* Now, we have valid params, go read the config space data */
    ks_write ((u16)hose->cfg_addr, address); 
    ks_write ((u16)hose->cfg_data, val); 

    return 0;
}

#if !defined( CONFIG_ARCH_KS8692P )
/*============================================================================
 * Initialize KS869x PCI Host Device (for USP )
 *============================================================================*/

void
pci_ks869x_hostDeviceUsb_init (void)
{
    unsigned long uReg;
    unsigned long cmd;


    /* KS8695P has 4 gpio pins for interrupt, device driver will set them accordingly */
    /* EXT0 is used as PCI bus interrupt, source level detection (active low) */

    uReg = ks_read(KS8695_GPIO_MODE);
    uReg |= 0x0000000f;		//at the begining, set them as output.
    ks_write(KS8695_GPIO_MODE, uReg);

    Pci_CfgWrite_dword (0, 5, 0, PCI_COMMAND, PCI_COMM_SETTING | PCI_COMM_IO );                
//    Pci_CfgWrite_dword (0, 5, 1, PCI_COMMAND, (PCI_COMM_SETTING & ~PCI_COMM_MEM ) | PCI_COMM_IO );                
    Pci_CfgWrite_dword (0, 5, 1, PCI_COMMAND, PCI_COMM_SETTING | PCI_COMM_IO );                
    Pci_CfgWrite_dword (0, 5, 2, PCI_COMMAND, PCI_COMM_SETTING | PCI_COMM_IO );                

    cmd = Pci_CfgRead_dword (0, 5, 0, PCI_BASE_ADDRESS_4);                

    cmd  += 0x200;
    Pci_CfgWrite_dword (0, 5, 1, PCI_BASE_ADDRESS_4, cmd);               

    Pci_CfgWrite_dword (0, 0, 0, PCI_INTERRUPT_LINE, 2 );	//IRQ 2                

    Pci_CfgWrite_dword (0, 5, 0, PCI_INTERRUPT_LINE, 2 );	//IRQ 2                
    Pci_CfgWrite_dword (0, 5, 1, PCI_INTERRUPT_LINE, 2 );       //IRQ 2         
    Pci_CfgWrite_dword (0, 5, 2, PCI_INTERRUPT_LINE, 2 );       //IRQ 2         

//    Pci_CfgWrite_dword (0, 5, 0, PCI_INTERRUPT_PIN, 1 );	//INTA                
//    Pci_CfgWrite_dword (0, 5, 1, PCI_INTERRUPT_PIN, 1 );        //INTA         
//    Pci_CfgWrite_dword (0, 5, 2, PCI_INTERRUPT_PIN, 1 );        //INTA         

}
#endif


/*============================================================================
 * Initialize KS869x PCI Host Bridge
 *============================================================================*/

void
pci_ks869x_hostBridge_init (void)
{

    /* initialize subid/subdevice  */
    //ks_write( REG_PCI_SYSID, ks_read( REG_PCI_CONFID ));
    ks_write( REG_PCI_SYSID, 0x00010001 );

    /* prefetch limits with 16 words, retry enable */
    ks_write( REG_PCI_CONTROL, PCI_BMEM_PREFLMIT16 );

#if (0)
    /* Set PCI-AHB Bridge Bus Mode to Host Bridge and PCI Mode */
    ks_write(REG_PCI_BMODE, ( PCI_HOST_BRIDGE | PCI_PCI_MODE ) );	
#endif

#if (1)
    /* Set PCI-AHB Bridge Bus Mode to Host Bridge and Mini PCI Mode */
    ks_write(REG_PCI_BMODE, ( PCI_HOST_BRIDGE | PCI_MINIPCI_MODE ) );	
#endif


    /* Configure memory mapping */

    /* memory map as seen by the CPU on the local bus to 0x10000000 */
    ks_write( REG_PCI_MEMBASE, PCI_CPU_MEM_BASE );

    /* mask bits 0xF8000000 */
    ks_write( REG_PCI_MBMARSK, PCI_MEMBASE_MASK );

    /* memory address (PCI view of PCI memory space, 1 to 1 translation) to 0x10000000  */
    ks_write( REG_PCI_MBTRANS, PCI_CPU_MEM_BASE );

    /* enable memory address translation */
    ks_write(REG_PCI_MEMBCTRL, PCI_ENABLE_ADDTRAN);

    /* Configure IO mapping */

    /* memory map as seen by the CPU on the local bus to 0x08000000 */
    ks_write( REG_PCI_IOBASE, PCI_CPU_IO_BASE );

	/* mask bits 0xFF800000 */
    ks_write( REG_PCI_IOBMARSK, PCI_IOBASE_MASK );

    /* PCI view of PCI space for PCI devices */
    ks_write( REG_PCI_IOBTRANS, 0);

    /* enable I/O address translation */
    ks_write(REG_PCI_IOBCTRL, PCI_ENABLE_ADDTRAN);

}


/*============================================================================
 * Initialize KS869x PCI Host Device 
 *============================================================================*/
#define L1_CACHE_SHIFT          5
#define L1_CACHE_BYTES          (1 << L1_CACHE_SHIFT)

void
pci_ks869x_hostDevice_init (void)
{
    /* set KS869x PCI host device memory base address to 0x10000000 */
    Pci_CfgWrite_dword (0, 0, 0, PCI_BASE_ADDRESS_0, PCI_CPU_MEM_BASE);  

    /* set KS869x PCI host device latency time to 0x20, cache line size to 8 dword */
    Pci_CfgWrite_dword (0, 0, 0, PCI_CACHE_LINE_SIZE, 
//                         (((PCI_LATENCY_TIMER_DEF << 8) & PCI_LATENCY_TIMER_MASK ) | ( PCI_CACHE_LINESZ & PCI_CACHE_LINESZ_MASK )));
                        (32 << 8) | (L1_CACHE_BYTES / sizeof(u32)));

    /* set KS869x PCI host device command to enable System error detect, 
       Parity error detect, Master operation, Memory space access */
    Pci_CfgWrite_dword (0, 0, 0, PCI_COMMAND, PCI_COMM_SETTING);                

#if !defined( CONFIG_ARCH_KS8692P )

    /* Initialize KS869x PCI Host Device (for USP ) */
    pci_ks869x_hostDeviceUsb_init ();
#endif
}


static void pci_config_ks869x
(
    struct pci_controller *hose, 
    pci_dev_t dev,
    struct pci_config_table *_
)
{

    /* Initialize KS869x PCI Host Bridge Configuration Registers (AHB Access) */
    pci_ks869x_hostBridge_init ();

   /* Initialize KS869x PCI Host Device Configuration Registers (PCI Access) */
    pci_ks869x_hostDevice_init ();
}

/*============================================================================
 * Configure KS869x PCI Host Bridge 
 *============================================================================*/

static struct pci_config_table ks869x_config_table[] = {
  { PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID,
    PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID,
    pci_config_ks869x },
  { }
};

static struct pci_controller ks869x_hose = {
  config_table: ks869x_config_table,
};

void
pci_ks869x_init(struct pci_controller *hose)
{

    hose = &ks869x_hose;

   /*
    * Initialize KS869x PCI Host Bridge Configuration Registers (AHB Access) 
    */

    pci_ks869x_hostBridge_init ();


   /*
    * Initialize KS869x PCI Host Device Configuration Registers (PCI Access) 
	* bus number and device number are both equal '0', to configure PCI host device itself.
    */

    pci_ks869x_hostDevice_init ();


   /* 
    * Initialize pci_controller struct 
    */

    hose->first_busno = 0;
    hose->last_busno = 0xff;

    /* Set PCI memory base address, size to pci_controller struct */
    pci_set_region(hose->regions + 0,
                   PCI_CPU_MEM_BASE,
		           PCI_MEM_PHYS,
		           PCI_MEM_SIZE,
		           PCI_REGION_MEM);


    /* Set PCI IO base address, size to pci_controller struct */
    pci_set_region(hose->regions + 1,
                   PCI_CPU_IO_BASE,
                   PCI_IO_PHYS,
                   PCI_IO_SIZE,
                   PCI_REGION_IO);

    hose->region_count = 2;

    /* Set PCI Configuration Address and Data Registers to pci_controller struct,
	   using these register to read\write data to PCI device */
    pci_setup_indirect(hose,
                       REG_PCI_CNFADD,
                       REG_PCI_CNGDATA );


    /* Set PCI read\write architecture-dependent routine to pci_controller struct */
    pci_set_ops(hose,
		        ks869x_pci_CfgRead_byte,
		        ks869x_pci_CfgRead_word,
		        ks869x_pci_CfgRead_dword,
		        ks869x_pci_CfgWrite_byte,
		        ks869x_pci_CfgWrite_word,
		        ks869x_pci_CfgWrite_dword);

    /*
     * Hose scan.
     */
    pci_register_hose(hose);

    hose->last_busno = pci_hose_scan(hose);
}

#endif /* CONFIG_PCI */
