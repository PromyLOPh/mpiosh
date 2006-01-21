/*
 * $Id: mpio.h,v 1.24 2006/01/21 18:33:20 germeier Exp $
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
int	mpio_memory_free(mpio_t *, mpio_mem_t, DWORD *free);

/* report sectors in block for this memory */
int     mpio_block_get_sectors(mpio_t *, mpio_mem_t);
/* report size of block for this memory */
int     mpio_block_get_blocksize(mpio_t *, mpio_mem_t);

/*
 * charset for filename encoding/converting
 */
CHAR  *mpio_charset_get(mpio_t *);
BYTE   mpio_charset_set(mpio_t *, CHAR *);

/* 
 * directory operations 
 */

/* context, memory bank */
BYTE*	mpio_directory_open(mpio_t *, mpio_mem_t);
/* context, memory bank, directory name */
int     mpio_directory_make(mpio_t *, mpio_mem_t, CHAR *);
/* context, memory bank, directory name */
int     mpio_directory_cd(mpio_t *, mpio_mem_t, CHAR *);
/* context, memory bank, directory name buffer space */
void    mpio_directory_pwd(mpio_t *, mpio_mem_t, CHAR pwd[INFO_LINE]);  
/* context, dir context */
BYTE*	mpio_dentry_next(mpio_t *, mpio_mem_t, BYTE *);
/* context, dir context */
int	mpio_dentry_get(mpio_t *, mpio_mem_t, BYTE *, CHAR *, int,WORD *,
			BYTE *, BYTE *, BYTE *, BYTE *, DWORD *, BYTE *);

/* 
 * reading/writing/deleting of files
 */

int	mpio_file_get_as(mpio_t *, mpio_mem_t, mpio_filename_t,
			 mpio_filename_t,mpio_callback_t); 

/* context, memory bank, filename, callback */
int	mpio_file_get(mpio_t *, mpio_mem_t, mpio_filename_t, mpio_callback_t); 

/* context, memory bank, filename, filetype, callback */
int	mpio_file_put(mpio_t *, mpio_mem_t, mpio_filename_t, mpio_filetype_t,
		      mpio_callback_t); 

/* context, memory bank, filename, as, filetype, callback */
int	mpio_file_put_as(mpio_t *, mpio_mem_t, mpio_filename_t,
			 mpio_filename_t, mpio_filetype_t,
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
				mpio_callback_t, CHAR **); 

/* context, memory bank, filename, filetype, callback ... */
/* ... memory pointer, size of file                       */
int	mpio_file_put_from_memory(mpio_t *, mpio_mem_t, mpio_filename_t, 
				  mpio_filetype_t, mpio_callback_t,
				  CHAR *, int);

/* check if file exists on selected memory */
/* return pointer to file dentry if file exists */
BYTE   *mpio_file_exists(mpio_t *, mpio_mem_t, mpio_filename_t);


/* 
 * rename a file on the MPIO
 */
/* context, memory bank, filename, filename */
int	mpio_file_rename(mpio_t *, mpio_mem_t, 
			 mpio_filename_t, mpio_filename_t);

/* 
 * switch position of two files
 */
/* context, memory bank, filename, filename */
int	mpio_file_switch(mpio_t *, mpio_mem_t, 
			 mpio_filename_t, mpio_filename_t);

/* Move a named file after a given file. If after==NULL move it
   to the top of the list,
*/
  
int mpio_file_move(mpio_t *,mpio_mem_t m,mpio_filename_t,mpio_filename_t);

/* 
 * formating a memory (internal mem or external SmartMedia card)
 */
  
/* context, memory bank, callback */
int	mpio_memory_format(mpio_t *, mpio_mem_t, mpio_callback_t); 

/* mpio_sync has to be called after every set of mpio_file_{del,put}
 * operations to write the current state of FAT and (root) directory.
 * This is done explicitly to avoid writing these informations to often
 * and thereby exhausting the SmartMedia chips
 */
/* context, memory bank */
int	mpio_sync(mpio_t *, mpio_mem_t);

/*
 * ID3 rewriting support
 */

/* enable disable ID3 rewriting support */
BYTE   mpio_id3_set(mpio_t *, BYTE);
/* query ID3 rewriting support */
BYTE   mpio_id3_get(mpio_t *);

/* set format string for rewriting*/
void   mpio_id3_format_set(mpio_t *, CHAR *);
/* get format string for rewriting*/
void   mpio_id3_format_get(mpio_t *, CHAR *);

/*
 * "special" functions
 */

/* returns health status of selected memory */
int     mpio_health(mpio_t *, mpio_mem_t, mpio_health_t *);

/* 
 * error handling
 */

/* returns error code of last error */
int 	mpio_errno(void);

/* returns the description of the error <errno> */
char *	mpio_strerror(int err);

/* prints the error message of the last error*/
void	mpio_perror(char *prefix);

/* set error code to given value */
int	mpio_error_set(int err);


/* 
 * debugging
 */
 
/* context, memory bank */
int	mpio_memory_dump(mpio_t *, mpio_mem_t);

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


