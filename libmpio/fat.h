/* 
 *
 * $Id: fat.h,v 1.1 2002/08/28 16:10:51 salmoon Exp $
 *
 * Library for USB MPIO-*
 *
 * Markus Germeier (mager@tzi.de)
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

#ifndef _MPIO_FAT_H_
#define _MPIO_FAT_H_

#include "defs.h"


int	mpio_bootblocks_read(mpio_t *, mpio_mem_t);

int	mpio_fat_read(mpio_t *, mpio_mem_t);
int	mpio_fat_write(mpio_t *, mpio_mem_t);
int	mpio_fat_clear(mpio_t *, mpio_mem_t);

int	mpio_fat_entry_read(mpio_t *, mpio_mem_t, int);
int	mpio_fat_entry_write(mpio_t *, mpio_mem_t, int, WORD);
int	mpio_fat_entry_free(mpio_t *, mpio_mem_t, int);

int	mpio_fat_free_clusters(mpio_t *, mpio_mem_t);
int	mpio_fat_find_free(mpio_t *, mpio_mem_t);

int	mpio_fat_internal_find_startsector(mpio_t *, BYTE);

#endif /* _MPIO_FAT_H_ */
