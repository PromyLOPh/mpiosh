/* -*- linux-c -*- */

/* 
 *
 * $Id: io.h,v 1.11 2002/10/13 08:57:31 germeier Exp $
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

/* phys.<->log. block mapping */
int   mpio_zone_init(mpio_t *, mpio_cmd_t);
/* context, memory bank, logical block */
/* returns address of physical block! */
DWORD mpio_zone_block_find_log(mpio_t *, mpio_cmd_t, DWORD);
 /* context, memory bank, sequential block (for sector addressing) */
/* returns address of physical block! */
DWORD mpio_zone_block_find_seq(mpio_t *, mpio_cmd_t, DWORD);
/* context, memory bank, logical block */
/* find used physical block and mark it as unused! */
/* returns address of physical block! (to delete the physical block!) */
DWORD mpio_zone_block_set_free(mpio_t *, mpio_cmd_t, DWORD);
/* context, memory bank, physical block */
void  mpio_zone_block_set_free_phys(mpio_t *, mpio_cmd_t, DWORD);
/* context, memory bank, physical block */
void  mpio_zone_block_set_defect_phys(mpio_t *, mpio_cmd_t, DWORD);
/* context, memory bank, logical block */
/* looks for a free physical block to be used for given logical block */
/* mark the found block used for this logical block */
/* returns address of physical block! */
DWORD mpio_zone_block_find_free_log(mpio_t *, mpio_cmd_t, DWORD);
DWORD mpio_zone_block_find_free_seq(mpio_t *, mpio_cmd_t, DWORD);
/* return zone-logical block for a given physical block */
WORD mpio_zone_block_get_logical(mpio_t *, mpio_cmd_t, DWORD);


/* real I/O */
int	mpio_io_set_cmdpacket(mpio_t *, mpio_cmd_t, mpio_mem_t, 
			      DWORD, BYTE, BYTE, BYTE *);

int	mpio_io_bulk_read (int, BYTE *, int);
int	mpio_io_bulk_write(int, BYTE *, int);

/* read version block into memory */
int	mpio_io_version_read(mpio_t *, BYTE *);

/* */
int	mpio_io_sector_read (mpio_t *, BYTE, DWORD, BYTE *);
int	mpio_io_sector_write(mpio_t *, BYTE, DWORD, BYTE *);

/* */
int	mpio_io_block_read  (mpio_t *, mpio_mem_t, mpio_fatentry_t *, BYTE *);
int	mpio_io_block_write (mpio_t *, mpio_mem_t, mpio_fatentry_t *, BYTE *);
int	mpio_io_block_delete(mpio_t *, mpio_mem_t, mpio_fatentry_t *);
/* needed for formatting of external memory */
int	mpio_io_block_delete_phys(mpio_t *, BYTE, DWORD);

/* */
int	mpio_io_spare_read  (mpio_t *, BYTE, DWORD, BYTE, BYTE, BYTE *, int,
			     mpio_callback_init_t);

#endif /* _MPIO_IO_H_ */
