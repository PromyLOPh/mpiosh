/*
 * $Id: io.h,v 1.3 2003/04/27 12:08:21 germeier Exp $
 *
 *  libmpio - a library for accessing Digit@lways MPIO players
 *  Copyright (C) 2002, 2003 Markus Germeier
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc.,g 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * uses code from mpio_stat.c by
 * Yuji Touya (salmoon@users.sourceforge.net)
 */

#ifndef _MPIO_IO_H_
#define _MPIO_IO_H_

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

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
/* */
void mpio_zone_block_set(mpio_t *, mpio_cmd_t, DWORD);

/* real I/O */
int	mpio_io_set_cmdpacket(mpio_t *, mpio_cmd_t, mpio_mem_t, 
			      DWORD, WORD, BYTE, BYTE *);

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
int	mpio_io_spare_read  (mpio_t *, BYTE, DWORD, WORD, BYTE, BYTE *, int,
			     mpio_callback_init_t);

#ifdef __cplusplus
}
#endif 

#endif /* _MPIO_IO_H_ */
