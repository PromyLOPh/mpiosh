/* -*- linux-c -*- */

/* 
 *
 * $Id: io.h,v 1.1 2002/08/28 16:10:51 salmoon Exp $
 *
 * Library for USB MPIO-*
 *
 * Markus Germeier (mager@tzi.de)
 *
 * uses code from mpio_stat.c by
 * Yuji Touya (salmoon@users.sourceforge.net)
 *
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * */

#ifndef _MPIO_IO_H_
#define _MPIO_IO_H_

#include "defs.h"

/* real I/O */
int	mpio_io_set_cmdpacket(mpio_cmd_t, mpio_mem_t,
			      DWORD, BYTE, BYTE, BYTE *);

int	mpio_io_bulk_read(int, BYTE *, int);
int	mpio_io_bulk_write(int, BYTE *, int);

/* read version block into memory */
int	mpio_io_version_read(mpio_t *, BYTE *);

/* */
int	mpio_io_sector_read(mpio_t *, BYTE, DWORD, BYTE, BYTE, BYTE *);
int	mpio_io_sector_write(mpio_t *, BYTE, DWORD, BYTE, BYTE, BYTE *);

/* */
int	mpio_io_block_read(mpio_t *, BYTE, DWORD, BYTE, BYTE, BYTE *);
int	mpio_io_block_write(mpio_t *, BYTE, DWORD, BYTE, BYTE, BYTE *);
int	mpio_io_block_delete(mpio_t *, BYTE, DWORD, BYTE);

/* */
int	mpio_io_spare_read  (mpio_t *, BYTE, DWORD, BYTE, BYTE, BYTE *, int);

#endif /* _MPIO_IO_H_ */
