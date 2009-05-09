/*
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

#ifndef _MPIO_FAT_H_
#define _MPIO_FAT_H_

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* generate fresh boot sectors for formatting of external mem */
BYTE    *mpio_mbr_gen(BYTE);
BYTE    *mpio_pbr_gen(BYTE);

/* only needed for external memory */
int	mpio_bootblocks_read(mpio_t *, mpio_mem_t);
int     mpio_mbr_eval(mpio_smartmedia_t *);
int     mpio_pbr_eval(mpio_smartmedia_t *);

/* functions on the FAT for internal *and* external */
int	mpio_fat_read(mpio_t *, mpio_mem_t, mpio_callback_init_t);
int	mpio_fat_write(mpio_t *, mpio_mem_t);
int	mpio_fat_clear(mpio_t *, mpio_mem_t);
int	mpio_fat_free_clusters(mpio_t *, mpio_mem_t);
int	mpio_fat_free(mpio_t *, mpio_mem_t);

/* functions to iterate through the FAT linked list(s) */
mpio_fatentry_t *mpio_fatentry_new(mpio_t *, mpio_mem_t, DWORD, BYTE);
int              mpio_fatentry_plus_plus(mpio_fatentry_t *);

mpio_fatentry_t *mpio_fatentry_find_free(mpio_t *, mpio_mem_t, BYTE);
int              mpio_fatentry_next_free(mpio_t *, mpio_mem_t, 
					 mpio_fatentry_t *);
int              mpio_fatentry_next_entry(mpio_t *, mpio_mem_t, 
					 mpio_fatentry_t *);
DWORD	         mpio_fatentry_read(mpio_t *, mpio_mem_t, mpio_fatentry_t *);
int	         mpio_fatentry_write(mpio_t *, mpio_mem_t, mpio_fatentry_t *,
				     WORD);

int              mpio_fatentry_set_free  (mpio_t *, mpio_mem_t, 
					  mpio_fatentry_t *);
int              mpio_fatentry_set_defect(mpio_t *, mpio_mem_t, 
					  mpio_fatentry_t *);
int              mpio_fatentry_set_eof(mpio_t *, mpio_mem_t, 
					  mpio_fatentry_t *);
int              mpio_fatentry_set_next(mpio_t *, mpio_mem_t, 
					mpio_fatentry_t *, mpio_fatentry_t *);
int              mpio_fatentry_is_defect(mpio_t *, mpio_mem_t, 
					  mpio_fatentry_t *);
int              mpio_fatentry_free(mpio_t *, mpio_mem_t, mpio_fatentry_t *);
  
/* finding a file is fundamental different for internal mem */
int	mpio_fat_internal_find_startsector(mpio_t *, BYTE);
BYTE	mpio_fat_internal_find_fileindex(mpio_t *);

/* mapping logical <-> physical for internal memory only */
void mpio_fatentry_hw2entry(mpio_t *,  mpio_fatentry_t *);
void mpio_fatentry_entry2hw(mpio_t *,  mpio_fatentry_t *);

#ifdef __cplusplus
}
#endif 

#endif /* _MPIO_FAT_H_ */
