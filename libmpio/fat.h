/* 
 *
 * $Id: fat.h,v 1.3 2002/09/08 23:22:48 germeier Exp $
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
int	mpio_fat_free_clusters(mpio_t *, mpio_mem_t);
int	mpio_fat_free(mpio_t *, mpio_mem_t);

mpio_fatentry_t *mpio_fatentry_new(mpio_t *, mpio_mem_t, DWORD);
int              mpio_fatentry_plus_plus(mpio_fatentry_t *);

mpio_fatentry_t *mpio_fatentry_find_free(mpio_t *, mpio_mem_t);
int              mpio_fatentry_next_free(mpio_t *, mpio_mem_t, 
					 mpio_fatentry_t *);
int              mpio_fatentry_next_entry(mpio_t *, mpio_mem_t, 
					 mpio_fatentry_t *);
DWORD	         mpio_fatentry_read(mpio_t *, mpio_mem_t, mpio_fatentry_t *);
int	         mpio_fatentry_write(mpio_t *, mpio_mem_t, mpio_fatentry_t *, 
				     WORD);

int	mpio_fat_internal_find_startsector(mpio_t *, BYTE);

void mpio_fatentry_hw2entry(mpio_t *,  mpio_fatentry_t *);
void mpio_fatentry_entry2hw(mpio_t *,  mpio_fatentry_t *);

#endif /* _MPIO_FAT_H_ */
