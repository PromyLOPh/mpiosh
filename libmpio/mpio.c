/* 
 *
 * $Id: mpio.c,v 1.9 2002/09/11 00:18:34 germeier Exp $
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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "defs.h"
#include "debug.h"
#include "directory.h"
#include "io.h"
#include "mpio.h"
#include "smartmedia.h"
#include "fat.h"

#define DSTRING 100

void mpio_init_internal(mpio_t *);
void mpio_init_external(mpio_t *);

void 
mpio_init_internal(mpio_t *m)
{
  mpio_smartmedia_t *sm = &(m->internal);
  BYTE i_offset         = 0x18;

  /* init main memory parameters */
  sm->manufacturer = m->version[i_offset];
  sm->id           = m->version[i_offset+1];
  sm->size         = mpio_id2mem(sm->id);
  sm->chips        = 1;
  
  /* look for a second installed memory chip */
  if (mpio_id_valid(m->version[i_offset+2]))
    {    
      if(mpio_id2mem(sm->id) != mpio_id2mem(m->version[i_offset+3]))
	{
	  printf("Found a MPIO with two chips with different size!");
	  printf("I'm utterly confused and aborting now, sorry!");
	  printf("Please report this to: mpio-devel@lists.sourceforge.net\n");
	  exit(1);	  
	}
    sm->size  = 2 * mpio_id2mem(sm->id);
    sm->chips = 2;
  }

  /* used for size calculations (see mpio_memory_free) */
  mpio_id2geo(sm->id, &sm->geo);

  /* read FAT information from spare area */  
  sm->max_cluster  = sm->size/16*1024;      /* 1 cluster == 16 KB */
  debugn(2,"max_cluster: %d\n", sm->max_cluster);
                                            /* 16 bytes per cluster */
  sm->fat_size     = sm->max_cluster*16/SECTOR_SIZE;   
  sm->fat          = malloc(sm->fat_size*SECTOR_SIZE);
  mpio_fat_read(m, MPIO_INTERNAL_MEM);

  /* Read directory from internal memory */
  sm->dir_offset=0;
  mpio_rootdir_read(m, MPIO_INTERNAL_MEM);
}

void
mpio_init_external(mpio_t *m)
{
  mpio_smartmedia_t *sm=&(m->external);
  BYTE e_offset=0x20;

  /* heuristic to find the right offset for the external memory */
  while((e_offset<0x3a) && !(mpio_id_valid(m->version[e_offset])))
    e_offset++;
  
  if (mpio_id_valid(m->version[e_offset]))
    {
      sm->manufacturer=m->version[e_offset];
      sm->id=m->version[e_offset+1];
    } else {
      sm->manufacturer=0;
      sm->id=0;
    }  

  /* init memory parameters if external memory is found */
  if (sm->id!=0) 
    {
      /* Read things from external memory (if available) */
      sm->size  = mpio_id2mem(sm->id);
      sm->chips = 1;                   /* external is always _one_ chip !! */

      mpio_id2geo(sm->id, &sm->geo);
      
      if (sm->size < 16) 
	{
	  debug("Sorry, I don't believe this software works with " 
		"SmartMedia Cards less than 16MB\n"
		"Proceed with care and send any findings to: "
		"mpio-devel@lists.sourceforge.net\n");
	}
      
      mpio_bootblocks_read(m, MPIO_EXTERNAL_MEM);
      sm->fat = malloc(SECTOR_SIZE*sm->fat_size);
      mpio_fat_read(m, MPIO_EXTERNAL_MEM);
      mpio_rootdir_read(m, MPIO_EXTERNAL_MEM);
    }
}

mpio_t *
mpio_init(void) 
{
  mpio_t *new_mpio;

  new_mpio = malloc(sizeof(mpio_t));
  if (!new_mpio) {
    debug ("Error allocating memory for mpio_t");
    return NULL;
  }  

  new_mpio->fd = open(MPIO_DEVICE, O_RDWR);

  if (new_mpio->fd < 0) {
    
    debug ("Could not open %s\n"
	   "Verify that the mpio module is loaded and "
	   "your MPIO is\nconnected and powered up.\n\n" , MPIO_DEVICE);
    
    return NULL;    
  }
  
  /* Read Version Information */
  mpio_io_version_read(new_mpio, new_mpio->version);  

  /* fill in values */
  /* TODO: check for different versions !! */
  snprintf(new_mpio->firmware.id, 12, "%s", new_mpio->version);
  snprintf(new_mpio->firmware.major, 3, "%s", new_mpio->version + 0x0c);
  snprintf(new_mpio->firmware.minor, 3, "%s", new_mpio->version + 0x0e);
  snprintf(new_mpio->firmware.year, 5, "%s", new_mpio->version + 0x10);
  snprintf(new_mpio->firmware.month, 3, "%s", new_mpio->version + 0x14);
  snprintf(new_mpio->firmware.day, 3, "%s", new_mpio->version + 0x16);

  /* internal init */
  mpio_init_internal(new_mpio);

  /* external init */
  mpio_init_external(new_mpio);

  return new_mpio;  
}

/* 
 * returns available memory
 *
 * free:  give back free memory size
 *
 */
int
mpio_memory_free(mpio_t *m, mpio_mem_t mem, int *free)
{
  if (mem==MPIO_INTERNAL_MEM) {    
    *free=mpio_fat_free_clusters(m, mem);    
    return (m->internal.geo.SumSector 
	    * SECTOR_SIZE / 1000 * m->internal.chips);    
  }
  
  if (mem==MPIO_EXTERNAL_MEM) {
    if (!m->external.size) {
      *free=0;
      return 0;
    }    
    *free=mpio_fat_free_clusters(m, mem);    
    return (m->external.geo.SumSector * SECTOR_SIZE / 1000);    
  }

  return 0;
}

void
mpio_close(mpio_t *m) 
{
  close(m->fd);
  
  if(m->internal.fat)
    free(m->internal.fat);
  if(m->external.fat)
    free(m->external.fat);
  
  free(m);
  
}

void    
mpio_get_info(mpio_t *m, mpio_info_t *info)
{
  int max=INFO_LINE-1;
  snprintf(info->firmware_id, max,      "\"%s\"", m->firmware.id);
  snprintf(info->firmware_version, max, "%s.%s", 
	   m->firmware.major, m->firmware.minor);
  snprintf(info->firmware_date, max,    "%s.%s.%s", 
	   m->firmware.day, m->firmware.month, m->firmware.year);
  
  if (m->internal.chips == 1) 
    {    
      snprintf(info->firmware_mem_internal, max, "%3dMB (%s)", 
	       mpio_id2mem(m->internal.id), 
	       mpio_id2manufacturer(m->internal.manufacturer));
    } else {
      snprintf(info->firmware_mem_internal, max, "%3dMB (%s) - %d chips", 
	       mpio_id2mem(m->internal.id)*m->internal.chips, 
	       mpio_id2manufacturer(m->internal.manufacturer),
	       m->internal.chips);
    }
  
  if (m->external.id)
    {      
      snprintf(info->firmware_mem_external, max, "%3dMB (%s)", 
	       mpio_id2mem(m->external.id), 
	       mpio_id2manufacturer(m->external.manufacturer));
    } else {
      snprintf(info->firmware_mem_external, max, "not available");
    }
      
}

int
mpio_file_get(mpio_t *m, mpio_mem_t mem, BYTE *filename, 
	      BYTE (*progress_callback)(int, int))
{
  mpio_smartmedia_t *sm;
  BYTE fname[129];
  BYTE block[BLOCK_SIZE];
  int fd, towrite;
  BYTE   *p;
  mpio_fatentry_t *f=0;
  
  DWORD filesize, fsize;

  BYTE abort = 0;
  
  /* please fix me sometime */
  /* the system entries are kind of special ! */
  if (strncmp("sysdum", filename, 6) == 0)
    return 0;  

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;  
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  /* find file */
  p = mpio_dentry_find_name(m, mem, filename);
  if (!p)
      p = mpio_dentry_find_name_8_3(m, mem, filename);

  if (p)
    f = mpio_dentry_get_startcluster(m, mem, p);      
  
  if (f && p) {    
    filesize=fsize=mpio_dentry_get_filesize(m, mem, p);
    
    unlink(filename);    
    fd = open(filename, (O_RDWR | O_CREAT), (S_IRWXU | S_IRGRP | S_IROTH));    
    
    do
      {
	debugn(2, "sector: %4x\n", f->entry);	    
    
	mpio_io_block_read(m, mem, f, block);

	if (filesize > BLOCK_SIZE) {
	  towrite = BLOCK_SIZE;
	} else {
	  towrite = filesize;
	}    
	
	if (write(fd, block, towrite) != towrite) {
	  debug("error writing file data\n");
	  close(fd);
	  return -1;
	} 
	filesize -= towrite;
	
	if (progress_callback)
	  abort=(*progress_callback)((fsize-filesize), fsize);

      } while ((mpio_fatentry_next_entry(m, mem, f) && (filesize>0)));
  
    close (fd);    
  } else {
    debug("unable to locate the file: %s\n", filename);
  }
  
  return (fsize-filesize);
}

int
mpio_file_put(mpio_t *m, mpio_mem_t mem, BYTE *filename, 
	      BYTE (*progress_callback)(int, int))
{
  mpio_smartmedia_t *sm;
  mpio_fatentry_t   *f, current, start;
  int data_offset;
  BYTE block[BLOCK_SIZE];
  int fd, toread;
  struct stat file_stat;

  BYTE *p=NULL;
  DWORD filesize, fsize, free;
  BYTE abort=0;
  
  if (mem==MPIO_INTERNAL_MEM) sm=&m->internal;  
  if (mem==MPIO_EXTERNAL_MEM) sm=&m->external;

  if (stat((const char *)filename, &file_stat)!=0) {
    debug("could not find file: %s\n", filename);
    return 0;
  }
  fsize=filesize=file_stat.st_size;
  debugn(2, "filesize: %d\n", fsize);

  /* check if there is enough space left */
  mpio_memory_free(m, mem, &free);
  if (free*1024<fsize) {
    debug("not enough space left (only %d KB)\n", free);
    return 0;
  }

  /* check if filename already exists */
  p = mpio_dentry_find_name(m, mem, filename);
  if (!p)
      p = mpio_dentry_find_name_8_3(m, mem, filename);
  if (p) 
    {
      debug("filename already exists\n");
      return 0;
    }

  /* find first free sector */
  f = mpio_fatentry_find_free(m, mem);
  if (!f) 
    {
      debug("could not free cluster for file!\n");
      return 0;	
    } else {
      memcpy(&start, f, sizeof(mpio_fatentry_t));
    }
  
  /* find file-id for internal memory */
  if (mem==MPIO_INTERNAL_MEM) 
    {      
      f->i_index=mpio_fat_internal_find_fileindex(m);      
      debug("fileindex: %02x\n", f->i_index);
    }

  /* open file for writing */
  fd = open(filename, O_RDONLY);    
  if (fd==-1) 
    {
      debug("could not open file: %s\n", filename);
      return 0;
    }

  while ((filesize>BLOCK_SIZE)) {

    if (filesize>=BLOCK_SIZE) {      
      toread=BLOCK_SIZE;
    } else {
      toread=filesize;
    }
    
    if (read(fd, block, toread)!=toread) {
      debug("error reading file data\n");
      close(fd);
      return -1;
    }
    filesize -= toread;

    /* get new free block from FAT and write current block out */
    memcpy(&current, f, sizeof(mpio_fatentry_t));    
    if (!(mpio_fatentry_next_free(m, mem, f))) 
      {
	debug ("Found no free cluster during mpio_file_put\n"
	       "This should never happen\n");
	exit(-1);
      }    
    mpio_fatentry_set_next(m ,mem, &current, f);
    mpio_io_block_write(m, mem, &current, block);
	
    if (progress_callback)
      abort=(*progress_callback)((fsize-filesize), fsize);
  }

  /* handle the last block to write */

  if (filesize>=BLOCK_SIZE) {      
    toread=BLOCK_SIZE;
  } else {
    toread=filesize;
  }

  if (read(fd, block, toread)!=toread) {
    debug("error reading file data\n");
    close(fd);
    return -1;
  }
  filesize -= toread;
  
  /* mark end of FAT chain and write last block */
  mpio_fatentry_set_eof(m ,mem, f);
  mpio_io_block_write(m, mem, f, block);

  close(fd);

  if (progress_callback)
    abort=(*progress_callback)((fsize-filesize), fsize);

  /* FIXEME: add real values here!!! */
  mpio_dentry_put(m, mem,
		  filename, strlen(filename),
		  2002, 8, 13,
		  2, 12, fsize, start.entry);

  /* this writes the FAT *and* the root directory */
  mpio_fat_write(m, mem);

  return fsize-filesize;
}

int
mpio_memory_format(mpio_t *m, mpio_mem_t mem, 
	      BYTE (*progress_callback)(int, int))
{
  int data_offset;
  mpio_smartmedia_t *sm;
  mpio_fatentry_t   *f;
  DWORD clusters;
  DWORD i;
  
  if (mem == MPIO_INTERNAL_MEM) 
    {    
      sm=&m->internal;
      data_offset = 0x01;
    }
  
  if (mem == MPIO_EXTERNAL_MEM) 
    {
      sm = &m->external;
      data_offset = 0x02;
    }

  clusters = sm->size*128;

  /* clear the fat before anything else, so we can mark clusters defective
   * if we found a bad block
   */
  mpio_fat_clear(m, mem);
  
  f = mpio_fatentry_new(m, mem, data_offset);  
  do 
    {
      if (!mpio_io_block_delete(m, mem, f))
	mpio_fatentry_set_defect(m, mem, f);
	
      if (progress_callback)
	(*progress_callback)(f->entry, sm->max_cluster + 1);
    } while (mpio_fatentry_plus_plus(f));
  free(f);

  if (mem == MPIO_EXTERNAL_MEM) {    
    /* format CIS area */
    f = mpio_fatentry_new(m, mem,        /* yuck */
			  (1-((sm->dir_offset + DIR_NUM)/BLOCK_SECTORS - 2)));
    mpio_io_block_delete(m, mem, f);
    free(f);
    mpio_io_sector_write(m, mem, 0x20, sm->cis);
    mpio_io_sector_write(m, mem, 0x21, sm->cis);    
  }

  mpio_rootdir_clear(m, mem);
  /* this writes the FAT *and* the root directory */
  mpio_fat_write(m, mem);

  if (progress_callback)
    (*progress_callback)(sm->max_cluster+1, sm->max_cluster+1);

  return 0;
}


int 
mpio_file_del(mpio_t *m, mpio_mem_t mem, BYTE *filename, 
	      BYTE (*progress_callback)(int, int))
{
  BYTE *p;
  mpio_smartmedia_t *sm;
  mpio_fatentry_t   *f, backup;
  DWORD filesize, fsize;
  BYTE abort=0;

  /* please fix me sometime */
  /* the system entry are kind of special ! */
  if (strncmp("sysdum", filename, 6)==0)
    return 0;  

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;  
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  /* find file */
  p = mpio_dentry_find_name(m, mem, filename);
  if (!p)
      p = mpio_dentry_find_name_8_3(m, mem, filename);

  if (p)
    f = mpio_dentry_get_startcluster(m, mem, p);      
  
  if (f && p) {    
    filesize=fsize=mpio_dentry_get_filesize(m, mem, p);    
    do
      {
	debugn(2, "sector: %4x\n", f->entry);	    
    
	mpio_io_block_delete(m, mem, f);
	
	if (filesize != fsize) 
	  mpio_fatentry_set_free(m, mem, &backup);

	memcpy(&backup, f, sizeof(mpio_fatentry_t));	

	if (filesize > BLOCK_SIZE) {
	  filesize -= BLOCK_SIZE;
	} else {
	  filesize -= filesize;
	}	
	
	if (progress_callback)
	  abort=(*progress_callback)((fsize-filesize), fsize);

      } while (mpio_fatentry_next_entry(m, mem, f));
    mpio_fatentry_set_free(m, mem, &backup);
  
  } else {
    debug("unable to locate the file: %s\n", filename);
  }

  mpio_dentry_delete(m, mem, filename);
  /* this writes the FAT *and* the root directory */
  mpio_fat_write(m, mem);
  
  return (fsize-filesize);
}


  
