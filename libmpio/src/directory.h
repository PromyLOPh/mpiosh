/* 
 *
 * $Id: directory.h,v 1.1 2003/04/23 08:34:14 crunchy Exp $
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

#ifndef _MPIO_DIRECTORY_H_
#define _MPIO_DIRECTORY_H_

#include "fat.h"
#include "time.h"

#ifdef __cplusplus
extern "C" {
#endif

/* root directory operations */
int     mpio_rootdir_read (mpio_t *, mpio_mem_t);
int	mpio_rootdir_clear (mpio_t *, mpio_mem_t);
int     mpio_rootdir_format(mpio_t *, mpio_mem_t);

/* directory opertations */
int     mpio_directory_init(mpio_t *, mpio_mem_t, mpio_directory_t *, 
			    WORD, WORD);
int     mpio_directory_read(mpio_t *, mpio_mem_t, mpio_directory_t *);
int     mpio_directory_write(mpio_t *, mpio_mem_t, mpio_directory_t *);
BYTE    mpio_directory_is_empty(mpio_t *, mpio_mem_t, mpio_directory_t *);

/* operations on a single directory entry */
int	mpio_dentry_get_size(mpio_t *, mpio_mem_t, BYTE *);
int	mpio_dentry_get_raw(mpio_t *, mpio_mem_t, BYTE *, BYTE *, int);
int	mpio_dentry_put(mpio_t *, mpio_mem_t, BYTE *, int,
			time_t, DWORD, WORD, BYTE);
BYTE *	mpio_dentry_find_name_8_3(mpio_t *, BYTE, BYTE *);
BYTE *	mpio_dentry_find_name(mpio_t *, BYTE, BYTE *);
int	mpio_dentry_delete(mpio_t *, BYTE, BYTE *);
int     mpio_dentry_get_filesize(mpio_t *, mpio_mem_t, BYTE *);
long    mpio_dentry_get_time(mpio_t *, mpio_mem_t, BYTE *);
mpio_fatentry_t    *mpio_dentry_get_startcluster(mpio_t *, mpio_mem_t, BYTE *);
BYTE    mpio_dentry_is_dir(mpio_t *, mpio_mem_t, BYTE *);

/* switch two directory entries */
void    mpio_dentry_switch(mpio_t *, mpio_mem_t, BYTE *, BYTE *);

/* rename a dentry */
void    mpio_dentry_rename(mpio_t *, mpio_mem_t, BYTE *, BYTE *);

/* Move a given file to a new position in the file
   list	relative to another file.	
*/
void	mpio_dentry_move(mpio_t *,mpio_mem_t, BYTE *, BYTE *);

/* helper functions */
void    mpio_dentry_copy_from_slot(BYTE *, mpio_dir_slot_t *);
void    mpio_dentry_copy_to_slot(BYTE *, mpio_dir_slot_t *);
int	mpio_dentry_get_real(mpio_t *, mpio_mem_t, BYTE *, BYTE *, 
			     int, BYTE[12],
			     WORD *, BYTE *, BYTE *, BYTE *, BYTE *, DWORD *,
			     BYTE *);

#ifdef __cplusplus
}
#endif 

#endif /* _MPIO_DIRECTORY_H_ */