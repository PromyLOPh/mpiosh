/* 
 *
 * $Id: mpio.c,v 1.3 2002/09/03 21:20:53 germeier Exp $
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
  sm->max_cluster  = sm->size/16*1024;
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
  sm->fat_size=sm->size*2;          /* the *2 is need here! */
  sm->fat=malloc(sm->fat_size*SECTOR_SIZE);
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
  *free=mpio_fat_free_clusters(m, mem);
  if (mem==MPIO_INTERNAL_MEM) {    
    return (m->internal.geo.SumSector 
	    * SECTOR_SIZE / 1000 * m->internal.chips);    
  }
  
  if (mem==MPIO_EXTERNAL_MEM) {
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
  
  if (m->internal.id)
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
  
  if (f) {    
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
  int data_offset;
  BYTE block[BLOCK_SIZE];
  DWORD startsector=0, sector, nextsector;
  DWORD realsector=0;
  int fd, toread;
  struct stat file_stat;
  
  DWORD filesize, fsize;
  DWORD fat_and;
  DWORD fat_end;  
  BYTE abort=0;
  
  debug("Support for writing files is deactivated!");
  return 0;

  if (mem==MPIO_INTERNAL_MEM) {    
    sm=&m->internal;
    data_offset=0x00;
    fat_and=0xffffffff;
    fat_end=0xffffffff;
    debug("writing to internal memory is not yet supported, sorry\n");    
    return 0;    
  }
  
  if (mem==MPIO_EXTERNAL_MEM) {
    sm=&m->external;
    data_offset=sm->dir_offset+DIR_NUM-(2*BLOCK_SECTORS);    
    /* FAT 16 vs. FAT 12 */  
    if (sm->size==128) {
      fat_and=0xffff;
      fat_end=0xfff8;
    } else {
      fat_and=0xfff;
      fat_end=0xff8;
    } 
  }

  if (stat((const char *)filename, &file_stat)!=0) {
    debug("could not find file: %s\n", filename);
    return 0;
  }
  
  fsize=filesize=file_stat.st_size;
  debugn(2, "filesize: %d\n", fsize);

  fd = open(filename, O_RDONLY);    
  if (fd==-1) {
    debug("could not find file: %s\n", filename);
    return 0;
  }

  while ((filesize>0) & (!abort)) {

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

    if (!startsector) {
      startsector=mpio_fat_find_free(m, mem);
      if (!startsector) {
	debug("could not find startsector for file!\n");
	close(fd);
	return 0;	
      }
      sector=startsector;
      debugn(2, "startsector: %d\n", startsector);
      /* evil hacks are us ;-) */
/*       mpio_fatentry_write(m, mem, startsector, fat_and); */
    } else {
      nextsector=mpio_fat_find_free(m, mem);
      if (!nextsector){
	debug("no more free blocks, short write!\n");
	close(fd);
	return (fsize-filesize);
      }
      debugn(2, " nextsector: %d\n", nextsector);

/*       mpio_fatentry_write(m, mem, sector, nextsector); */
      sector=nextsector;
      /* evil hacks are us ;-) */
/*       mpio_fatentry_write(m, mem, sector, fat_and); */

    }

    if (mem==MPIO_INTERNAL_MEM) {    
/*       realsector=fat_internal_find_startsector(m, sector);       */
/*       realsector=realsector+         /\* chose the right bank *\/   */
/* 	(0x01000000*((realsector/(sm->size/16*1000/sm->chips))+1));     */
/*       sector=realsector; */
/*       debugn(2, "startsector (real): %4x\n", sector);   */
    } else {
/*       realsector=sector_hack(sm->size, sector); */
    }

/*     debug("mager: %04x : %3d\n", realsector*BLOCK_SECTORS, realsector); */
    mpio_io_block_delete(m, mem, ((realsector*BLOCK_SECTORS) + data_offset),
			 sm->size);
    mpio_io_block_write(m, mem, realsector + (data_offset/BLOCK_SECTORS), 
			sm->size, block);

    filesize -= toread;
	
    if (progress_callback)
      abort=(*progress_callback)((fsize-filesize), fsize);
  }

  close(fd);

  /* FIXEME: add real values here!!! */
  mpio_dentry_put(m, mem,
		  filename, strlen(filename),
		  2002, 8, 13,
		  2, 12, fsize, startsector);

  mpio_fat_write(m, mem);

  return fsize-filesize;
}

int
mpio_memory_format(mpio_t *m, mpio_mem_t mem, 
	      BYTE (*progress_callback)(int, int))
{
  int data_offset;
  mpio_smartmedia_t *sm;
  DWORD clusters;
  DWORD i;
  
  if (mem == MPIO_INTERNAL_MEM) {    
    sm=&m->internal;
    data_offset=0x00;
    debug("formatting of internal memory is not yet supported, sorry\n");
    return 0;
  }
  
  if (mem == MPIO_EXTERNAL_MEM) {
    sm = &m->external;
    data_offset = sm->dir_offset+DIR_NUM;    
  }
/*   debug("mager: %2x\n", data_offset); */

  clusters = sm->size*128;
  
/*   debug("Clusters: %4x\n", sm->size*128); */

  for (i = data_offset; i < clusters; i += 0x20) {
    mpio_io_block_delete(m, mem, i, sm->size);
    if (progress_callback)
      (*progress_callback)(i, clusters + 1);
  }

  /* format CIS area */
  mpio_io_block_delete(m, mem, 0x20, sm->size);
  mpio_io_sector_write(m, mem, 0x20, sm->cis);
  mpio_io_sector_write(m, mem, 0x21, sm->cis);

  mpio_fat_clear(m, mem);
  mpio_rootdir_clear(m, mem);
  mpio_fat_write(m, mem);

  if (progress_callback)
    (*progress_callback)(clusters+1, clusters+1);

  return 0;
}


/* can we share code with mpio_file_get ??? */
int 
mpio_file_del(mpio_t *m, mpio_mem_t mem, BYTE *filename, 
	      BYTE (*progress_callback)(int, int))
{
  BYTE *p;
  BYTE bdummy;
  WORD wdummy;

  mpio_smartmedia_t *sm;
  int data_offset;
  BYTE fname[129];
  DWORD startsector=0, sector;
  DWORD realsector=0, oldsector=0;
  int towrite;
  
  DWORD filesize, fsize;
  DWORD fat_and;
  DWORD fat_end;  
  BYTE abort=0;

  debug("Support for deleting files is deactivated!");
  return 0;
  
  /* please fix me sometime */
  /* the system entry are kind of special ! */
  if (strncmp("sysdum", filename, 6)==0)
    return 0;  

  if (mem==MPIO_INTERNAL_MEM) {    
    sm=&m->internal;
    data_offset=0x00;
    fat_and=0xffffffff;
    fat_end=0xffffffff;
    debug("deleting from files of internal memory"
	  " is not yet supported, sorry\n");
    return 0;
  }
  
  if (mem==MPIO_EXTERNAL_MEM) {
    sm=&m->external;
    data_offset=sm->dir_offset+DIR_NUM-(2*BLOCK_SECTORS);    
    /* FAT 16 vs. FAT 12 */  
    if (sm->size==128) {
      fat_and=0xffff;
      fat_end=0xfff8;
    } else {
      fat_and=0xfff;
      fat_end=0xff8;
    } 
  }


  p=mpio_directory_open(m, mem);
  while (p) {

    mpio_dentry_get (m, p,
		     fname, 128,
		     &wdummy, &bdummy, &bdummy,
		     &bdummy, &bdummy, &filesize);
    fsize=filesize;
    if ((strcmp(fname,filename)==0) && (strcmp(filename,fname)==0)) {
      startsector=mpio_dentry_get_startcluster(m, mem, p);      
      p=NULL;
    }
    
    p=mpio_dentry_next(m, p);
  }
  
  /* grr, of curse these are clusters not sectors */
  /* must code while awake, must ... */
  if (startsector) {    
    debugn(2, "startsector: %4x\n", startsector);  
    sector = startsector;
    if (mem == MPIO_INTERNAL_MEM) {    
      realsector = mpio_fat_internal_find_startsector(m, sector);      
      realsector = realsector+         /* chose the right bank */  
	(0x01000000*((realsector / (sm->size / 16 * 1000 / sm->chips)) + 1));
      sector = realsector;
      debugn(2, "startsector (real): %4x\n", sector);  
    } else {
/*       realsector = sector_hack(sm->size, sector); */
    }

    mpio_io_block_delete(m, mem, (realsector * BLOCK_SECTORS) + data_offset,
			 sm->size);

    if (filesize > BLOCK_SIZE) {
      towrite = BLOCK_SIZE;
    } else {
      towrite = filesize;
    }    
    
    filesize -= towrite;

/*     debugn(5, "sector: (%6x) %4x\n",  */
/* 	  (sector&0xffffff), mpio_fatentry_read(m, mem, sector&0xffffff)); */

/*     while((((mpio_fatentry_read(m, mem, (sector & 0xffffff)) & fat_and) */
/* 	    < fat_end)) && (filesize>0) && (!abort)) {     */
/* 	oldsector = sector; */
/* 	sector = mpio_fatentry_read(m, mem, sector & 0xffffff);	 */
/* 	mpio_fatentry_write(m, mem, oldsector, 0); */
/* 	debugn(2,"next sector: %4x\n", sector);  */
/* 	if (mem == MPIO_INTERNAL_MEM) {     */
/* 	  realsector = sector; */
/* 	} else { */
/* 	  realsector = sector_hack(sm->size, sector); */
/* 	} */
	    
/* 	debugn(2," realsector: %4x\n", realsector);  */
	
/* 	mpio_io_block_delete(m, mem, */
/* 			     ((realsector * BLOCK_SECTORS) + data_offset), */
/* 			     sm->size); */
/* 	if (filesize > BLOCK_SIZE) { */
/* 	  towrite = BLOCK_SIZE; */
/* 	} else { */
/* 	  towrite = filesize; */
/* 	}     */
	
/* 	filesize -= towrite; */
	
/* 	if (progress_callback) */
/* 	  abort=(*progress_callback)((fsize-filesize), fsize); */
/*       } */
/*     mpio_fatentry_write(m, mem, sector, 0); */

  } else {
    debug("unable to locate the file: %s\n", filename);
  }

  /* TODO: delete directory entry */
  mpio_dentry_delete(m, mem, filename);

  mpio_fat_write(m, mem);
  
  return (fsize-filesize);
}


  
