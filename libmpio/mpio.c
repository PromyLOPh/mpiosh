/* 
 *
 * $Id: mpio.c,v 1.45 2003/03/25 23:56:52 germeier Exp $
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
#include <sys/types.h>
#include <utime.h>

#include "cis.h"
#include "defs.h"
#include "debug.h"
#include "directory.h"
#include "io.h"
#include "mpio.h"
#include "smartmedia.h"
#include "fat.h"

void mpio_bail_out(void);
void mpio_init_internal(mpio_t *);
void mpio_init_external(mpio_t *);
int  mpio_check_filename(mpio_filename_t);

int mpio_file_get_real(mpio_t *, mpio_mem_t, mpio_filename_t, mpio_filename_t,
		       mpio_callback_t, BYTE **); 

int mpio_file_put_real(mpio_t *, mpio_mem_t, mpio_filename_t, mpio_filename_t,
		       mpio_filetype_t, mpio_callback_t, BYTE *, int);

static BYTE *mpio_model_name[] = {
  "MPIO-DME",
  "MPIO-DMG",
  "MPIO-DMG+",
  "MPIO-DMB",
  "MPIO-DMB+",
  "MPIO-DMK",
  "MPIO-FD100",
  "MPIO-FL100",
  "MPIO-FY100",
  "unknown"
};

static mpio_error_t mpio_errors[] = {
  { MPIO_ERR_FILE_NOT_FOUND,
    "The selected file can not be found." },
  { MPIO_ERR_NOT_ENOUGH_SPACE,
    "There is not enough space left on the selected memory card." },
  { MPIO_ERR_FILE_EXISTS,
    "The selected file already exists and can not be overwritten. Remove it first." },
  { MPIO_ERR_FAT_ERROR,
    "Internal error while reading the FAT." },
  { MPIO_ERR_READING_FILE,
    "The selected file can not be read." },
  { MPIO_ERR_PERMISSION_DENIED,
    "There are not enough rights to access the file/directory." },
  { MPIO_ERR_WRITING_FILE,
    "There are no permisson to write to the selected file." },
  { MPIO_ERR_INT_STRING_INVALID,
    "Internal Error: Supported is invalid!" } 	
};

static const int mpio_error_num = sizeof mpio_errors / sizeof(mpio_error_t);

static int _mpio_errno = 0;

#define MPIO_ERR_RETURN(err) { _mpio_errno = err; return -1 ; }

#define MPIO_CHECK_FILENAME(filename) \
  if (!mpio_check_filename(filename)) { \
    MPIO_ERR_RETURN(MPIO_ERR_INT_STRING_INVALID); \
  }

void
mpio_bail_out(void){
  printf("I'm utterly confused and aborting now, sorry!");
  printf("Please report this to: mpio-devel@lists.sourceforge.net\n");
  exit(1);	  
}
  
int
mpio_check_filename(mpio_filename_t filename)
{
  BYTE *p=filename;
  
  while (p < (filename + MPIO_FILENAME_LEN))
    {
      if (*p)
	return 1;
      p++;
    }

  return 0;
}

void 
mpio_init_internal(mpio_t *m)
{
  mpio_smartmedia_t *sm = &(m->internal);
  BYTE i_offset         = 0x18;

  /* init main memory parameters */
  sm->manufacturer = m->version[i_offset];
  sm->id           = m->version[i_offset + 1];
  sm->chips        = 1;
  if (mpio_id_valid(m->version[i_offset])) 
    {      
      sm->size         = mpio_id2mem(sm->id);
    } else {
      sm->manufacturer = 0;
      sm->id           = 0;
      sm->size         = 0;
      debug("WARNING: no internal memory found\n");
      return;
    }
  
  
  /* look for further installed memory chips 
   * models with 1, 2 and 4 chips are known
   * (there _really_ shouldn't be any other versions out there ...)
   */
  while ((sm->chips < 4) && 
	 (mpio_id_valid(m->version[i_offset + (2 * sm->chips)])))
    {    
      if(mpio_id2mem(sm->id) != 
	 mpio_id2mem(m->version[i_offset + (2 * sm->chips) + 1]))
	{
	  printf("Found a MPIO with internal chips of different sizes!");
	  mpio_bail_out();
	}
      sm->chips++;
    }
	 
  if ((sm->chips == 3) || (sm->chips > 4)) 
    {
      printf("Found a MPIO with %d internal chips", sm->chips);
      mpio_bail_out();
    }
  
  sm->size  = sm->chips * mpio_id2mem(sm->id);

  /* used for size calculations (see mpio_memory_free) */
  mpio_id2geo(sm->id, &sm->geo);

  /* read FAT information from spare area */  
  sm->max_cluster  = sm->size / 16 * 1024;      /* 1 cluster == 16 KB */
  sm->max_blocks   = sm->max_cluster;
  debugn(2, "max_cluster: %d\n", sm->max_cluster);
                                            /* 16 bytes per cluster */
  sm->fat_size     = sm->max_cluster * 16 / SECTOR_SIZE;   
  debugn(2, "fat_size: %04x\n", sm->fat_size * SECTOR_SIZE);
  sm->fat          = malloc(sm->fat_size * SECTOR_SIZE);
  /* fat will be read in mpio_init, so we can more easily handle
     a callback function */

  /* Read directory from internal memory */
  sm->dir_offset=0;
  mpio_rootdir_read(m, MPIO_INTERNAL_MEM);
}

void
mpio_init_external(mpio_t *m)
{
  mpio_smartmedia_t *sm = &(m->external);
  BYTE e_offset = 0x20;

  /* heuristic to find the right offset for the external memory */
  while((e_offset < 0x3a) && !(mpio_id_valid(m->version[e_offset])))
    e_offset++;
  
  if ((mpio_id_valid(m->version[e_offset])) &&
      (m->model != MPIO_MODEL_FL100)) /* ignore external memory ATM until
					 we know how to support it! */
    {
      sm->manufacturer = m->version[e_offset];
      sm->id = m->version[e_offset + 1];
    } else {
      sm->manufacturer = 0;
      sm->id = 0;
      sm->chips = 0;
      sm->size = 0;
    }  

  /* init memory parameters if external memory is found */
  if (sm->id != 0) 
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
      
      /* for reading the spare area later! */
      sm->max_blocks  = sm->size / 16 * 1024;      /* 1 cluster == 16 KB */
      sm->spare       = malloc(sm->max_blocks * 0x10);
    }
}

mpio_t *
mpio_init(mpio_callback_init_t progress_callback) 
{
  mpio_t *new_mpio;
  mpio_smartmedia_t *sm;  
  int id_offset;
  BYTE i;

  new_mpio = malloc(sizeof(mpio_t));
  if (!new_mpio) {
    debug ("Error allocating memory for mpio_t");
    return NULL;
  }  
  memset(new_mpio, 0, sizeof(mpio_t));

  new_mpio->fd = open(MPIO_DEVICE, O_RDWR);

  if (new_mpio->fd < 0) {
    
    debug("Could not open %s\n"
	  "Verify that the mpio module is loaded and "
	  "your MPIO is\nconnected and powered up.\n\n" , MPIO_DEVICE);
    
    return NULL;    
  }
  
  /* Read Version Information */
  mpio_io_version_read(new_mpio, new_mpio->version);  

  /* fill in values */
  snprintf(new_mpio->firmware.id,   12, "%s", new_mpio->version);
  /* fix for newer versions which have a 0x00 in their version */
  for (i=0x0c ; i<0x10 ; i++)
    if (new_mpio->version[i] == 0x00)
      new_mpio->version[i]=' ';
  snprintf(new_mpio->firmware.major, 3, "%s", new_mpio->version + 0x0c);
  snprintf(new_mpio->firmware.minor, 3, "%s", new_mpio->version + 0x0e);
  snprintf(new_mpio->firmware.year,  5, "%s", new_mpio->version + 0x10);
  snprintf(new_mpio->firmware.month, 3, "%s", new_mpio->version + 0x14);
  snprintf(new_mpio->firmware.day,   3, "%s", new_mpio->version + 0x16);

  /* there are different identification strings! */
  if (strncmp(new_mpio->version, "MPIO", 4) == 0) {
    /* strings: "MPIOxy    " */
    id_offset = 4;

    /* string: "MPIO-xy   " */
    if (new_mpio->version[id_offset] == '-')
      id_offset++;

    /* identify different versions */
    switch (new_mpio->version[id_offset])
      {
      case 'E': 
	new_mpio->model = MPIO_MODEL_DME;
        break ;
      case 'K':
	new_mpio->model = MPIO_MODEL_DMK;
        break ;
      case 'G':
	new_mpio->model = MPIO_MODEL_DMG;
	if (new_mpio->version[id_offset+1] == 'P')
	  new_mpio->model = MPIO_MODEL_DMG_PLUS;
        break ;
      case 'B':
	new_mpio->model = MPIO_MODEL_DMB;
	if (new_mpio->version[id_offset+1] == 'P')
	  new_mpio->model = MPIO_MODEL_DMB_PLUS;
	break;	
      default:
	new_mpio->model = MPIO_MODEL_UNKNOWN;
      }
  } else if (strncmp(new_mpio->version, "FD100", 5) == 0) {
    new_mpio->model = MPIO_MODEL_FD100;
    debug("FD100 found: Beware, this model is not tested and we don't know"
	  " if it does work!\n");    
  } else if (strncmp(new_mpio->version, "FL100", 5) == 0) {
    /* we assume this model is not supported */
    new_mpio->model = MPIO_MODEL_FL100;
    debug("FL100 found: External memory is ignored, because we don't know how"
	  " to support it at the moment (MultiMediaCards instead of SmartMedia)\n");
  } else if (strncmp(new_mpio->version, "FY100", 5) == 0) {
    new_mpio->model = MPIO_MODEL_FY100;
    /* I assume this is like the FD100/FL100, I didn't had the chance to 
       look at a FY100 firmware yet. -mager */
    debug("FY100 found: Beware, this model is not tested and we don't know"
	  " if it does work!\n");    
  } else {
    new_mpio->model = MPIO_MODEL_UNKNOWN;
  }

  if (new_mpio->model == MPIO_MODEL_UNKNOWN) {
    debug("Unknown version string found!\n"
	  "Please report this to: mpio-devel@lists.sourceforge.net\n");
    hexdumpn(1, new_mpio->version, CMD_SIZE);
  }  
  
  /* internal init */
  mpio_init_internal(new_mpio);

  /* external init */
  mpio_init_external(new_mpio);

  /* read FAT/spare area */
  if (new_mpio->internal.id)
    mpio_fat_read(new_mpio, MPIO_INTERNAL_MEM, progress_callback);
  
  /* read the spare area (for block mapping) */
  if (new_mpio->external.id)
    {      
      sm = &new_mpio->external;  
      mpio_io_spare_read(new_mpio, MPIO_EXTERNAL_MEM, 0,
			 sm->size, 0, sm->spare,
			 (sm->max_blocks * 0x10), progress_callback);
      mpio_zone_init(new_mpio, MPIO_EXTERNAL_MEM);

      /* card might be defect */
      if (mpio_bootblocks_read(new_mpio, MPIO_EXTERNAL_MEM)==0) 
	{
	  sm->fat = malloc(SECTOR_SIZE*sm->fat_size);
	  mpio_fat_read(new_mpio, MPIO_EXTERNAL_MEM, NULL);
	  mpio_rootdir_read(new_mpio, MPIO_EXTERNAL_MEM);
	}
    }

  /* set default charset for filename conversion */
  new_mpio->charset=strdup(MPIO_CHARSET);
  
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
    if (!m->internal.size) {
      *free=0;
      return 0;
    }    
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
  if (m) {
    close(m->fd);
    
    if(m->internal.fat)
      free(m->internal.fat);
    if(m->external.fat)
      free(m->external.fat);
    
    free(m);
  }
}

mpio_model_t
mpio_get_model(mpio_t *m)
{
  mpio_model_t r;

  if (m) 
    {
      r = m->model;
    } else {
      r = MPIO_MODEL_UNKNOWN;
    }
  
  return r;
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
  snprintf(info->model, max, "%s", mpio_model_name[m->model]);
  
  if (!m->internal.id) 
    {
      snprintf(info->mem_internal, max, "not available");
    } else {      
      if (m->internal.chips == 1) 
	{    
	  snprintf(info->mem_internal, max, "%3dMB (%s)", 
		   mpio_id2mem(m->internal.id), 
		   mpio_id2manufacturer(m->internal.manufacturer));
	} else {
	  snprintf(info->mem_internal, max, "%3dMB (%s) - %d chips", 
		   mpio_id2mem(m->internal.id)*m->internal.chips, 
		   mpio_id2manufacturer(m->internal.manufacturer),
		   m->internal.chips);
	}
    }
  
  if (m->external.id)
    {      
      snprintf(info->mem_external, max, "%3dMB (%s)", 
	       mpio_id2mem(m->external.id), 
	       mpio_id2manufacturer(m->external.manufacturer));
    } else {
      snprintf(info->mem_external, max, "not available");
    }
      
}


int
mpio_file_get_as(mpio_t *m, mpio_mem_t mem, mpio_filename_t filename,
		 mpio_filename_t as, mpio_callback_t progress_callback)
{
  return mpio_file_get_real(m, mem, filename, as, progress_callback, NULL);
}

int
mpio_file_get(mpio_t *m, mpio_mem_t mem, mpio_filename_t filename, 
	      mpio_callback_t progress_callback)
{
  return mpio_file_get_real(m, mem, filename, NULL, progress_callback, NULL);
}

int
mpio_file_get_to_memory(mpio_t *m, mpio_mem_t mem, mpio_filename_t filename, 
			mpio_callback_t progress_callback, BYTE **memory)
{
  return mpio_file_get_real(m, mem, filename, NULL, progress_callback, memory);
}

int
mpio_file_get_real(mpio_t *m, mpio_mem_t mem, mpio_filename_t filename,
		   mpio_filename_t as, mpio_callback_t progress_callback,
		   BYTE **memory)
{
  mpio_smartmedia_t *sm;
  BYTE block[BLOCK_SIZE];
  int fd, towrite;
  BYTE   *p;
  mpio_fatentry_t *f = 0;
  struct utimbuf utbuf;
  long mtime;
  DWORD filesize, fsize;
  BYTE abort = 0;
  int merror;

  MPIO_CHECK_FILENAME(filename);

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;  
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  if(as==NULL) {
    as = filename;
  }

  /* find file */
  p = mpio_dentry_find_name(m, mem, filename);
  if (!p)
    p = mpio_dentry_find_name_8_3(m, mem, filename);

  if (p) 
    f = mpio_dentry_get_startcluster(m, mem, p);
  
  if (f && p) {    
    filesize=fsize=mpio_dentry_get_filesize(m, mem, p);

    if (memory) 
      {
	*memory = malloc(filesize);
      } else {
	unlink(filename);    
	fd = open(as, (O_RDWR | O_CREAT), (S_IRWXU | S_IRGRP | S_IROTH));
      }
    
    do
      {
	mpio_io_block_read(m, mem, f, block);

	if (filesize > BLOCK_SIZE) {
	  towrite = BLOCK_SIZE;
	} else {
	  towrite = filesize;
	}    

	if (memory)
	  {
	    memcpy((*memory)+(fsize-filesize) , block, towrite);
	  } else {
	    if (write(fd, block, towrite) != towrite) {
	      debug("error writing file data\n");
	      close(fd);
	      free (f);
	      MPIO_ERR_RETURN(MPIO_ERR_WRITING_FILE);
	    } 
	  }
	
	filesize -= towrite;
	
	if (progress_callback)
	  abort=(*progress_callback)((fsize-filesize), fsize);
	if (abort)
	  debug("aborting operation");	

      } while ((((merror=(mpio_fatentry_next_entry(m, mem, f)))>0) && 
		(filesize>0)) && (!abort));

    if (merror<0)
      debug("defective block encountered!\n");
  
    if(!memory) 
      {	
	close (fd);    
	free (f);
      }

    /* read and copied code from mtools-3.9.8/mcopy.c
     * to make this one right 
     */
    mtime=mpio_dentry_get_time(m, mem, p);
    utbuf.actime  = mtime;
    utbuf.modtime = mtime;
    utime(filename, &utbuf);

  } else {
    debug("unable to locate the file: %s\n", filename);
  }

  return (fsize-filesize);
}

int
mpio_file_put_as(mpio_t *m, mpio_mem_t mem, mpio_filename_t filename,
		 mpio_filename_t as, mpio_filetype_t filetype,
		 mpio_callback_t progress_callback)
{
  return mpio_file_put_real(m, mem, filename, as, filetype, 
			    progress_callback, NULL,0);
}

int
mpio_file_put(mpio_t *m, mpio_mem_t mem, mpio_filename_t filename, 
	      mpio_filetype_t filetype, mpio_callback_t progress_callback)
{
  return mpio_file_put_real(m, mem, filename, NULL, filetype, 
			    progress_callback, NULL,0);
}

int
mpio_file_put_from_memory(mpio_t *m, mpio_mem_t mem, mpio_filename_t filename, 
			  mpio_filetype_t filetype,
			  mpio_callback_t progress_callback,
			  BYTE *memory, int memory_size)
{
  return mpio_file_put_real(m, mem, filename,  NULL, filetype,
			    progress_callback, memory, memory_size);
}

int
mpio_file_put_real(mpio_t *m, mpio_mem_t mem, mpio_filename_t i_filename,
		   mpio_filename_t o_filename, mpio_filetype_t filetype,
		   mpio_callback_t progress_callback,
		   BYTE *memory, int memory_size)
{
  mpio_smartmedia_t *sm;
  mpio_fatentry_t   *f, current, firstblock, backup; 
  WORD start;
  BYTE block[BLOCK_SIZE];
  int fd, toread;
  struct stat file_stat;
  struct tm tt;

  BYTE *p = NULL;
  DWORD filesize, fsize, free, blocks;
  BYTE abort = 0;

  if(o_filename == NULL) {
    o_filename = i_filename;
  }

  MPIO_CHECK_FILENAME(o_filename);
  
  if (mem==MPIO_INTERNAL_MEM) sm=&m->internal;  
  if (mem==MPIO_EXTERNAL_MEM) sm=&m->external;

  if (memory)
    {
      fsize=filesize=memory_size;
      tt = (struct tm){ 0, 0, 0, 0, 0, 0, 0, 0, 0};
    } else {      
      if (stat((const char *)i_filename, &file_stat)!=0) {
	debug("could not find file: %s\n", i_filename);
	MPIO_ERR_RETURN(MPIO_ERR_FILE_NOT_FOUND);
      }
      fsize=filesize=file_stat.st_size;
      debugn(2, "filesize: %d\n", fsize);
    }
  
  /* check if there is enough space left */
  mpio_memory_free(m, mem, &free);
  if (free*1024<fsize) {
    debug("not enough space left (only %d KB)\n", free);
    MPIO_ERR_RETURN(MPIO_ERR_NOT_ENOUGH_SPACE);
  }

  /* check if filename already exists */
  p = mpio_dentry_find_name(m, mem, o_filename);
  if (!p)
      p = mpio_dentry_find_name_8_3(m, mem, o_filename);
  if (p) 
    {
      debug("filename already exists\n");
      MPIO_ERR_RETURN(MPIO_ERR_FILE_EXISTS);
    }

  /* find first free sector */
  f = mpio_fatentry_find_free(m, mem, filetype);
  if (!f) 
    {
      debug("could not free cluster for file!\n");
      MPIO_ERR_RETURN(MPIO_ERR_FAT_ERROR);
    } else {
      memcpy(&firstblock, f, sizeof(mpio_fatentry_t));
      start=f->entry;
    }  

  /* find file-id for internal memory */
  if (mem==MPIO_INTERNAL_MEM) 
    {      
      f->i_index=mpio_fat_internal_find_fileindex(m);
      debugn(2, "fileindex: %02x\n", f->i_index);
      f->i_fat[0x01]= f->i_index;
      if (m->model >= MPIO_MODEL_FD100) 
	f->i_fat[0x0e] = f->i_index;	
      start         = f->i_index;

      /* number of blocks needed for file */
      blocks = filesize / 0x4000;
      if (filesize % 0x4000)
	blocks++;      
      debugn(2, "blocks: %02x\n", blocks);      
      f->i_fat[0x02]=(blocks / 0x100) & 0xff;
      f->i_fat[0x03]= blocks          & 0xff;
    }  

  if (!memory)
    {      
      /* open file for writing */
      fd = open(i_filename, O_RDONLY);    
      if (fd==-1) 
	{
	  debug("could not open file: %s\n", i_filename);
	  MPIO_ERR_RETURN(MPIO_ERR_FILE_NOT_FOUND);
	}
    }

  while ((filesize>BLOCK_SIZE) && (!abort)) {

    if (filesize>=BLOCK_SIZE) {      
      toread=BLOCK_SIZE;
    } else {
      toread=filesize;
    }
    
    if (memory) 
      {
	memcpy(block, memory+(fsize-filesize), toread);
      } else {	
	if (read(fd, block, toread)!=toread) {
	  debug("error reading file data\n");
	  close(fd);
	  MPIO_ERR_RETURN(MPIO_ERR_READING_FILE);
	}
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

  if (memory)
    {
      memcpy(block, memory+(fsize-filesize), toread);
    } else {      
      if (read(fd, block, toread)!=toread) {
	debug("error reading file data\n");
	close(fd);
	MPIO_ERR_RETURN(MPIO_ERR_READING_FILE);
      }
    }
  filesize -= toread;
  
  /* mark end of FAT chain and write last block */
  mpio_fatentry_set_eof(m ,mem, f);
  mpio_io_block_write(m, mem, f, block);

  if (!memory)
    close(fd);

  if (progress_callback)
    (*progress_callback)((fsize-filesize), fsize);

  if (abort) 
    {    /* delete the just written blocks, because the user
	  * aborted the operation
	  */
      debug("aborting operation\n");	
      debug("removing already written blocks\n");      
      
      filesize=fsize;
      
      memcpy(&current, &firstblock, sizeof(mpio_fatentry_t));
      memcpy(&backup, &firstblock, sizeof(mpio_fatentry_t));
      
      while (mpio_fatentry_next_entry(m, mem, &current))
	{
	  mpio_io_block_delete(m, mem, &backup);
	  mpio_fatentry_set_free(m, mem, &backup);      
	  memcpy(&backup, &current, sizeof(mpio_fatentry_t));

	  if (filesize > BLOCK_SIZE) 
	    {
	      filesize -= BLOCK_SIZE;
	    } else {
	      filesize -= filesize;
	    }	
	  
	  if (progress_callback)
	    (*progress_callback)((fsize-filesize), fsize);
	}
      mpio_io_block_delete(m, mem, &backup);
      mpio_fatentry_set_free(m, mem, &backup);      

      if (filesize > BLOCK_SIZE) 
	{
	  filesize -= BLOCK_SIZE;
	} else {
	  filesize -= filesize;
	}	
      
      if (progress_callback)
	(*progress_callback)((fsize-filesize), fsize);
      
    } else {
      mpio_dentry_put(m, mem,
		      o_filename, strlen(o_filename),
		      ((memory)?mktime(&tt):file_stat.st_ctime), 
		      fsize, start);
    }  

  return fsize-filesize;
}

int	
mpio_file_switch(mpio_t *m, mpio_mem_t mem, 
		 mpio_filename_t file1, mpio_filename_t file2)
{
  BYTE *p1, *p2;

  /* find files */
  p1 = mpio_dentry_find_name(m, mem, file1);
  if (!p1)
    p1 = mpio_dentry_find_name_8_3(m, mem, file1);

  p2 = mpio_dentry_find_name(m, mem, file2);
  if (!p2)
    p2 = mpio_dentry_find_name_8_3(m, mem, file2);

  if ((!p1)  || (!p2))
    MPIO_ERR_RETURN(MPIO_ERR_FILE_NOT_FOUND);

  mpio_dentry_switch(m, mem, p1, p2);

  return 0;
}

int mpio_file_move(mpio_t *m,mpio_mem_t mem, mpio_filename_t file,
		   mpio_filename_t after)
{
  BYTE *p1,*p2;

  if( ( p1 = mpio_dentry_find_name(m,mem,file)) == NULL ) {
    if( (p1 = mpio_dentry_find_name_8_3(m, mem, file)) == NULL ) {
      MPIO_ERR_RETURN(MPIO_ERR_FILE_NOT_FOUND);
    }
  }
  
  if(after!=NULL) {
    if( ( p2 = mpio_dentry_find_name(m,mem,after)) == NULL ) {
      if( (p2 = mpio_dentry_find_name_8_3(m, mem, after)) == NULL ) {
	MPIO_ERR_RETURN(MPIO_ERR_FILE_NOT_FOUND);
      }
    }
  }

  fprintf(stderr," -- moving '%s' after '%s'\n",file,after);

  mpio_dentry_move(m,mem,p1,p2);

  return 0;
}

int
mpio_memory_format(mpio_t *m, mpio_mem_t mem,
		   mpio_callback_t progress_callback)
{
  int data_offset;
  mpio_smartmedia_t *sm;
  mpio_fatentry_t   *f;
  DWORD clusters;
  BYTE abort = 0;
  BYTE *cis, *mbr, *pbr;
  BYTE defect[SECTOR_SIZE];
  int i;

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

  
  /* TODO: read and write "Config.dat" so the player does not become "dumb" */
  if (mem==MPIO_INTERNAL_MEM) 
    {
      /* clear the fat before anything else, so we can mark clusters defective
       * if we found a bad block
       */
      /* external mem must be handled diffrently, 
       * see comment(s) below!
       */
      mpio_fat_clear(m, mem);
      f = mpio_fatentry_new(m, mem, data_offset, FTYPE_MUSIC);  
      do 
	{
	  
	  if (!mpio_io_block_delete(m, mem, f))
	    mpio_fatentry_set_defect(m, mem, f);
	  
	  if (progress_callback)
	    {
	      if (!abort) 
		{	      
		  abort=(*progress_callback)(f->entry, sm->max_cluster + 1);
		  if (abort)
		    debug("received abort signal, but ignoring it!\n");
		} else {
		  (*progress_callback)(f->entry, sm->max_cluster + 1);
		}	      
	    }      
	  
	} while (mpio_fatentry_plus_plus(f));
      free(f);
    }

  if (mem == MPIO_EXTERNAL_MEM) {    
    /* delete all blocks! */

    i=0; 
    while (i < sm->max_blocks)
      {
	mpio_io_block_delete_phys(m, mem, (i * BLOCK_SECTORS));	
	i++;

	if (progress_callback)
	  {
	    if (!abort) 
	      {	      
		abort=(*progress_callback)(i, sm->max_blocks);
		if (abort)
		  debug("received abort signal, but ignoring it!\n");
	      } else {
		(*progress_callback)(i, sm->max_blocks);
	      }	      
	  }      


      }

    /* generate "defect" first block */
    mpio_io_sector_write(m, mem, MPIO_BLOCK_DEFECT, defect);
    
    /* format CIS area */
    f = mpio_fatentry_new(m, mem, MPIO_BLOCK_CIS, FTYPE_MUSIC);
    /*     mpio_io_block_delete(m, mem, f); */
    free(f);
    cis = (BYTE *)mpio_cis_gen(); /* hmm, why this cast? */
    mpio_io_sector_write(m, mem, MPIO_BLOCK_CIS,   cis);
    mpio_io_sector_write(m, mem, MPIO_BLOCK_CIS+1, cis);    
    free(cis);

    /* generate boot blocks ... */
    mbr = (BYTE *)mpio_mbr_gen(m->external.size);
    pbr = (BYTE *)mpio_pbr_gen(m->external.size);
    /* ... copy the blocks to internal memory structurs ... */
    memcpy(sm->cis, cis, SECTOR_SIZE);
    memcpy(sm->mbr, mbr, SECTOR_SIZE);
    memcpy(sm->pbr, pbr, SECTOR_SIZE);
    /* ... and set internal administration accordingly */
    mpio_mbr_eval(sm);
    mpio_pbr_eval(sm);

    if (!sm->fat) 		/* perhaps we have to build a new FAT */
      sm->fat=malloc(sm->fat_size*SECTOR_SIZE);
    mpio_fat_clear(m, mem);

    /* TODO: for external memory we have to work a little "magic"
     * to identify and mark "bad blocks" (because we have a set of
     * spare ones!
     */
  }

  mpio_rootdir_clear(m, mem);

  /* this writes the FAT *and* the root directory */
  mpio_fat_write(m, mem);

  if (progress_callback)
    (*progress_callback)(sm->max_cluster+1, sm->max_cluster+1);

  return 0;
}

int 
mpio_file_del(mpio_t *m, mpio_mem_t mem, mpio_filename_t filename, 
	      mpio_callback_t progress_callback)
{
  BYTE *p;
  mpio_smartmedia_t *sm;
  mpio_fatentry_t   *f, backup;
  DWORD filesize, fsize;
  BYTE abort=0;

  MPIO_CHECK_FILENAME(filename);

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

	if (filesize > BLOCK_SIZE) 
	  {
	    filesize -= BLOCK_SIZE;
	  } else {
	    filesize -= filesize;
	  }	
	
	if (progress_callback)
	  {	    
	    if (!abort) 
	      {		
		abort=(*progress_callback)((fsize-filesize), fsize);
		if (abort)
		  debug("received abort signal, but ignoring it!\n");
	      } else {
		(*progress_callback)((fsize-filesize), fsize);
	      }	    
	  }
	
      } while (mpio_fatentry_next_entry(m, mem, f));
/*     mpio_io_block_delete(m, mem, &backup); */
    mpio_fatentry_set_free(m, mem, &backup);
    free(f);
  
  } else {
    debug("unable to locate the file: %s\n", filename);
  }

  mpio_dentry_delete(m, mem, filename);
  
  return (fsize-filesize);
}

int	
mpio_sync(mpio_t *m, mpio_mem_t mem)
{
  mpio_smartmedia_t *sm;
  
  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;  
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  if (!sm->size)
    return 0;

  /* this writes the FAT *and* the root directory */
  return mpio_fat_write(m, mem);  
}

int
mpio_memory_dump(mpio_t *m, mpio_mem_t mem)
{
  BYTE block[BLOCK_SIZE];
  int i;

  if (mem == MPIO_INTERNAL_MEM) 
    {      
      hexdump(m->internal.fat, m->internal.max_blocks*0x10);
      hexdump(m->internal.dir, DIR_SIZE);
    }
  
  if (mem == MPIO_EXTERNAL_MEM) 
    {      
      hexdump(m->external.spare, m->external.max_blocks*0x10);
      hexdump(m->external.fat,   m->external.fat_size*SECTOR_SIZE);
      hexdump(m->external.dir, DIR_SIZE);
    }
  
  for (i = 0 ; i<=0x100 ; i++) 
    mpio_io_sector_read(m, mem, i, block);
  
  return 0;  
}

int
mpio_errno(void)
{
  int no = _mpio_errno;
  _mpio_errno = 0;
  
  return no;
}

char *
mpio_strerror(int err)
{
  int i;

  if (err >= 0) return NULL;
  
  for (i = 0; i < mpio_error_num; i++) {
    if (mpio_errors[i].id == err)
      return mpio_errors[i].msg;
  }

  return NULL;
}

void
mpio_perror(char *prefix)
{
  char *msg = mpio_strerror(_mpio_errno);

  if (msg == NULL) return;
  
  if (prefix)
    fprintf(stderr, "%s: %s\n", prefix, msg);
  else
    fprintf(stderr, "%s\n", msg);
}
