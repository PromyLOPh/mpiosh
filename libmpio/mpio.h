#/* -*- linux-c -*- */

/* 
 * $Id: mpio.h,v 1.13 2002/11/13 23:05:28 germeier Exp $
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

#ifndef _MPIO_H_
#define _MPIO_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 
 *init and shutdown 
 */

mpio_t *mpio_init(mpio_callback_init_t);
void	mpio_close(mpio_t *);

/*
 * request information
 */

/* get version */
void	mpio_get_info(mpio_t *, mpio_info_t *);
/* get model */
mpio_model_t	mpio_get_model(mpio_t *);
/* retrieves free memory in bytes */
int	mpio_memory_free(mpio_t *, mpio_mem_t, int *free);

/*
 * charset for filename encoding/converting
 */
BYTE  *mpio_charset_get(mpio_t *);
BYTE   mpio_charset_set(mpio_t *, BYTE *);

/* 
 * directory operations 
 */

/* context, memory bank */
BYTE*	mpio_directory_open(mpio_t *, mpio_mem_t);
/* context, dir context */
BYTE*	mpio_dentry_next(mpio_t *, mpio_mem_t, BYTE *);
/* context, dir context */
int	mpio_dentry_get(mpio_t *, mpio_mem_t, BYTE *, BYTE *, int,WORD *,
			BYTE *, BYTE *, BYTE *, BYTE *, DWORD *);

/* 
 * reading/writing/deleting of files
 */

/* context, memory bank, filename, callback */
int	mpio_file_get(mpio_t *, mpio_mem_t, mpio_filename_t, mpio_callback_t); 

/* context, memory bank, filename, filetype, callback */
int	mpio_file_put(mpio_t *, mpio_mem_t, mpio_filename_t, mpio_filetype_t,
		      mpio_callback_t); 

/* context, memory bank, filename, callback */
int	mpio_file_del(mpio_t *, mpio_mem_t, mpio_filename_t, mpio_callback_t); 

/* 
 * reading/writing files into memory (used for config+font files)
 */

/* context, memory bank, filename, callback, pointer to memory     */
/* the needed memory is allocated and the memory pointer is return */
/* via the "BYTE **"                                               */
  
int	mpio_file_get_to_memory(mpio_t *, mpio_mem_t, mpio_filename_t, 
				mpio_callback_t, BYTE **); 

/* context, memory bank, filename, filetype, callback ... */
/* ... memory pointer, size of file                       */
int	mpio_file_put_from_memory(mpio_t *, mpio_mem_t, mpio_filename_t, 
				  mpio_filetype_t, mpio_callback_t,
				  BYTE *, int); 

/* 
 * switch position of two files
 */
/* context, memory bank, filename, filename */
int	mpio_file_switch(mpio_t *, mpio_mem_t, 
			 mpio_filename_t, mpio_filename_t);

/* 
 * formating a memory (internal mem or external SmartMedia card)
 */

/* context, memory bank, callback */
int	mpio_memory_format(mpio_t *, mpio_mem_t, mpio_callback_t); 

/* mpio_sync has to be called after every set of mpio_file_{del,put}
 * operations to write the current state of FAT and root directory.
 * This is done explicitly to avoid writing these informations to often
 * and thereby exhausting the SmartMedia chips
 */
/* context, memory bank */
int	mpio_sync(mpio_t *, mpio_mem_t); 

/* context, memory bank */
int	mpio_memory_dump(mpio_t *, mpio_mem_t); 

/* 
 * error handling
 */

/* returns error code of last error */
int 	mpio_errno(void);

/* returns the description of the error <errno> */
char *	mpio_strerror(int err);

/* prints the error message of the last error*/
void	mpio_perror(char *prefix);

/* 
 * timeline:
 * ---------
 * 2004: some functions to change the order of files
 * 2005: read mp3 tags of the files
 */

#ifdef __cplusplus
}
#endif 
  
#endif /* _MPIO_H_ */


