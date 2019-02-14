/*
 * (C) Copyright 2009 Micrel, Inc.
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

#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_NAND)

#include <asm/io.h>
#include <asm/errno.h>
#include <nand.h>
#include <asm/arch/ks8692Reg.h>
#include <asm/arch/platform.h>


/* ------------------------------------------------------------------------- */

#define NAND_HW_READ( b, a )  readl(( void* )(( char* )( b ) + a ))
#define NAND_HW_WRITE( b, a, d )  writel( d, ( void* )(( char* )( b ) + a ))
#define NAND_BUSY()  readl(( void* )( KS8692_IO_BASE + KS8692_NAND_BUSY_STATUS ))


#define NAND_READY_CMD  0x10


int nand_max_bank = 1;

static int nand_bank = 0;

static int nand_page_shift = 0;
static int nand_bank_shift = 0;
static int nand_oob_shift = 0;
static int nand_size_shift = 0;

static int nand_read_ptr = 0;
static u_char nand_data_byte[ 4 ];
static u_int* nand_data_ptr = ( u_int* ) nand_data_byte;

static int ready_command_set = 0;
static int last_command = 0;

/* ------------------------------------------------------------------------- */

static int nand_dev_ready(struct mtd_info *mtd)
{
	return( !( NAND_BUSY() & ( 1 << nand_bank )));
}

/**
 * nand_read_byte - [DEFAULT] read one byte from the chip
 * @mtd:	MTD device structure
 *
 * Default read function for 8bit buswith
 */
static u_char nand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;

	nand_read_ptr &= 3;
	if ( !nand_read_ptr ) {
		*nand_data_ptr = readl( this->IO_ADDR_R );
	}
	return nand_data_byte[ nand_read_ptr++ ];
}

static void nand_write_cmd(struct mtd_info *mtd, u_int data)
{
	struct nand_chip *this = mtd->priv;
	int need_ready_command = 0;

	/* Need to write a special command to make hardware accept read/write
	   commands.
	*/
	if ( NAND_READY_CMD == data )
		ready_command_set = 1;
	else if ( NAND_CMD_READ0 == data  ||
			NAND_CMD_READ1 == data  ||
			NAND_CMD_SEQIN == data  ||
			NAND_CMD_READOOB == data )
		need_ready_command = 1;
	if ( need_ready_command  &&  !ready_command_set ) {
		writel( NAND_READY_CMD, this->IO_ADDR_W );
		ready_command_set = 1;
	}
	last_command = data;

	/* Following commands have bank parameter. */
	switch ( data ) {
		case NAND_CMD_RESET:
		case NAND_CMD_STATUS:
		case NAND_CMD_READID:
			data |= nand_bank << NAND_CMD_BANK_SHIFT;
			break;
	}

	writel( data, this->IO_ADDR_W );

	/* Following commands will cause hardware not to accept read/write
	   commands.
	*/
	if ( NAND_CMD_READ0 == data  ||
			NAND_CMD_READ1 == data  ||
			NAND_CMD_SEQIN == data  ||
			NAND_CMD_READOOB == data )
		ready_command_set = 0;

	/* Need to write to data register to send read command. */
	if ( NAND_CMD_READ0 == data  ||
			NAND_CMD_READ1 == data  ||
			NAND_CMD_READOOB == data )
		writel( 0, this->IO_ADDR_R );

	/* Reset read data pointer. */
	nand_read_ptr = 0;

	/* A hack to return the right IDs when there are 5 instead of 4. */
	if ( NAND_CMD_READID == ( data & 0xFF ) ) {
		udelay( 10 );
		*nand_data_ptr = readl( this->IO_ADDR_R );
		data = NAND_HW_READ( this->IO_ADDR_R, KS8692_NAND_ID0 );
		while ( nand_read_ptr < 4  &&
				nand_data_byte[ nand_read_ptr ] !=
				( data & 0xFF ) ) {
			nand_read_ptr++;
			data >>= 8;
		}
	}
}

/**
 * nand_command - [DEFAULT] Send command to NAND device
 * @mtd:	MTD device structure
 * @command:	the command to be sent
 * @column:	the column address for this command, -1 if none
 * @page_addr:	the page address for this command, -1 if none
 *
 * Send command to NAND device. This function is used for small page
 * devices (256/512 Bytes per page)
 */
static void nand_command (struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
	register struct nand_chip *this = mtd->priv;

	if (column != -1 || page_addr != -1) {
		u_int data;
		u_int nand_column;
		u_int nand_page;

		nand_column = nand_page = 0;
		data = 0;

		if (column != -1) {
			if (column >= mtd->oobblock &&
					!( this->options &
					NAND_SAMSUNG_LP_OPTIONS )) {
				/* OOB area */
				column -= mtd->oobblock;
				if ( nand_oob_shift > 31 )
					data = 1;
				else
					nand_column = 1 << nand_oob_shift;
			}

			/* Emulate NAND_CMD_READOOB */
			if (command == NAND_CMD_READOOB  &&
					( this->options &
					NAND_SAMSUNG_LP_OPTIONS )) {
				column += mtd->oobblock;
				command = NAND_CMD_READ0;
			}

			/* Adjust columns for 16 bit buswidth */
			if (this->options & NAND_BUSWIDTH_16)
				column >>= 1;
			nand_column |= column;
		}
		if (page_addr != -1) {
			nand_page = page_addr;
		}

		if ( nand_bank_shift >= 31  &&
				nand_max_bank < 4 )
			NAND_HW_WRITE( this->IO_ADDR_R,
				KS8692_NAND_EXT_INDEX,
				nand_bank >> 1 );
		else if ( nand_oob_shift > 31 )
			NAND_HW_WRITE( this->IO_ADDR_R,
				KS8692_NAND_EXT_INDEX,
				data );

		data = ( nand_page << nand_page_shift ) + nand_column +
			( nand_bank << nand_bank_shift );
		NAND_HW_WRITE( this->IO_ADDR_R, KS8692_NAND_INDEX, data );
	}

	/*
	 * Write out the command to the device.
	 */
	/* A hack to read Samsung chips that require a second 0x30 command. */
	if ( ( this->options & NAND_SAMSUNG_LP_OPTIONS )  &&
			NAND_CMD_READ0 == command ) {
		command = NAND_CMD_CE_OFF;
	}
	nand_write_cmd(mtd, command);

	/*
	 * program and erase have their own busy handlers
	 * status and sequential in needs no delay
	*/
	switch (command) {

	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_STATUS:
		return;

	case NAND_CMD_RESET:
		if (this->dev_ready)
			break;
		udelay(this->chip_delay);
		nand_write_cmd(mtd, NAND_CMD_STATUS);
		while (!(this->read_byte(mtd) & NAND_STATUS_READY)) ;
		return;

	/* This applies to read commands */
	default:
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		*/
		if (!this->dev_ready) {
			udelay (this->chip_delay);
			return;
		}
	}

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay (100);
	/* wait until command is processed */
	while (!this->dev_ready(mtd));
}

/**
 * nand_select_chip - [DEFAULT] control CE line
 * @mtd:	MTD device structure
 * @chip:	chipnumber to select, -1 for deselect
 *
 * Default select function for 1 chip devices.
 */
static void nand_select_chip(struct mtd_info *mtd, int chip)
{
	struct nand_chip *this = mtd->priv;

	/* A hack to avoid detecing wrong number of chips. */
	if ( nand_bank != chip  &&  chip != -1 ) {
		if ( NAND_CMD_READID == last_command ) {
			writel( NAND_CMD_STATUS, this->IO_ADDR_W );
			last_command = 0;
		}
	}
	nand_bank = chip;
	if ( !( this->options & NAND_SAMSUNG_LP_OPTIONS ) )
		this->options &= ~NAND_NO_AUTOINCR;
	switch (chip) {
	case -1:
		NAND_HW_WRITE( this->IO_ADDR_R,
			KS8692_NAND_COMMAND, NAND_CMD_CE_OFF );
		nand_bank = 0;
		break;

	default:
		if ( chip > nand_max_bank )
			BUG();
		NAND_HW_WRITE( this->IO_ADDR_R,
			KS8692_NAND_COMMAND, NAND_CMD_CE_OFF );
		udelay(100);
		NAND_HW_WRITE( this->IO_ADDR_R,
			KS8692_NAND_COMMAND, NAND_CMD_CE_OFF );
		udelay(10);
		NAND_HW_WRITE( this->IO_ADDR_R, KS8692_NAND_INDEX,
			nand_bank << nand_bank_shift );
		NAND_HW_READ( this->IO_ADDR_R, KS8692_NAND_DATA );
	}
}

/**
 * nand_write_buf - [DEFAULT] write buffer to chip
 * @mtd:	MTD device structure
 * @buf:	data buffer
 * @len:	number of bytes to write
 *
 * Default write function for 8bit buswith
 */
static void nand_write_buf(struct mtd_info *mtd, const u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;
	u_int* data = ( u_int* ) buf;

	for ( i = 0; i < len; i += 4 ) {
		writel( *data++, this->IO_ADDR_R );
	}
}

/**
 * nand_read_buf - [DEFAULT] read chip data into buffer
 * @mtd:	MTD device structure
 * @buf:	buffer to store date
 * @len:	number of bytes to read
 *
 * Default read function for 8bit buswith
 */
static void nand_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;
	u_int* data = ( u_int* ) buf;

	for ( i = 0; i < len; i += 4 ) {
		*data++ = readl( this->IO_ADDR_R );
	}
}

/**
 * nand_verify_buf - [DEFAULT] Verify chip data against buffer
 * @mtd:	MTD device structure
 * @buf:	buffer containing the data to compare
 * @len:	number of bytes to compare
 *
 * Default verify function for 8bit buswith
 */
static int nand_verify_buf(struct mtd_info *mtd, const u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;
	u_int* data = ( u_int* ) buf;

	for ( i = 0; i < len; i += 4 ) {
		if ( *data++ != readl( this->IO_ADDR_R ) ) {
			return -EFAULT;
		}
	}

	return 0;
}


void board_nand_init(struct nand_chip *nand)
{
	u_int data;

	data = NAND_HW_READ( nand->IO_ADDR_R, KS8692_NAND_CONF );

	nand_size_shift = data & NAND_SIZE_8GBIT;
	if ( ( data & NAND_LARGE_BLOCK ) ) {
		nand_page_shift = 12;
		nand_bank_shift = 24 + nand_size_shift;
		if ( ( data & NAND_WIDTH_16BIT ) ) {
			nand_page_shift--;
			nand_bank_shift--;
		}
		nand->options |= NAND_SAMSUNG_LP_OPTIONS;
	}
	else {
		nand_page_shift = 9;
		nand_bank_shift = 23 + nand_size_shift;
		nand_oob_shift = nand_bank_shift + 2;
	}
	if ( ( data & NAND_WIDTH_16BIT ) ) {
		nand->options |= NAND_BUSWIDTH_16;
	}

	if ( ( data & NAND_4_BANK ) ) {
		nand_max_bank = 4;
	}
	else if ( ( data & NAND_2_BANK ) ) {
		nand_max_bank = 2;
	}

	/* Need read first for interface to accept command. */
	NAND_HW_READ( nand->IO_ADDR_R, KS8692_NAND_DATA );

	NAND_HW_WRITE( nand->IO_ADDR_R, KS8692_NAND_COMMAND, NAND_CMD_CE_OFF );

	nand->IO_ADDR_W = ( void* )(( char* )( nand->IO_ADDR_R ) +
		KS8692_NAND_COMMAND );

	nand->eccmode = NAND_ECC_SOFT;
	nand->dev_ready = nand_dev_ready;
	nand->select_chip = nand_select_chip;
	nand->chip_delay = 10;
	nand->cmdfunc = nand_command;
	nand->read_byte = nand_read_byte;
	nand->read_buf = nand_read_buf;
	nand->write_buf = nand_write_buf;
	nand->verify_buf = nand_verify_buf;
}
#endif

