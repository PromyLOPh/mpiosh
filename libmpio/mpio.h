/* -*- linux-c -*- */

/* 
 * $Id: mpio.h,v 1.4 2002/09/15 12:03:23 germeier Exp $
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

/* 
 *init and shutdown 
 */

mpio_t *mpio_init(void);
void	mpio_close(mpio_t *);

/*
 * request information
 */

/* get version */
void	mpio_get_info(mpio_t *, mpio_info_t *);
/* print version: deprecated */
//void    mpio_print_version(mpio_t *);
/* retrieves free memory in bytes */
int	mpio_memory_free(mpio_t *, mpio_mem_t, int *free);

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

/* context, memory bank, filename, callback */
int	mpio_file_get(mpio_t *, mpio_mem_t, BYTE *, BYTE (*)(int, int)); 

/* context, memory bank, filename, callback */
int	mpio_file_put(mpio_t *, mpio_mem_t, BYTE *, BYTE (*)(int, int)); 

/* context, memory bank, filename, callback */
int	mpio_file_del(mpio_t *, mpio_mem_t, BYTE *, BYTE (*)(int, int));

/* context, memory bank, callback */
int	mpio_memory_format(mpio_t *, mpio_mem_t, BYTE (*)(int, int)); 

/* mpio_sync has to be called after every set of mpio_file_{del,put}
 * operations to write the current state of FAT and root directory.
 * This is done explicitly to avoid writing these informations to often
 * and thereby exhausting the SmartMedia chips
 */
/* context, memory bank */
int	mpio_sync(mpio_t *, mpio_mem_t); 

/* context, memory bank */
int	mpio_memory_debug(mpio_t *, mpio_mem_t); 

/* 
 * timeline:
 * ---------
 * 2004: some functions to change the order of files
 * 2005: read mp3 tags of the files
 */
  
#endif /* _MPIO_H_ */


