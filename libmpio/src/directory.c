/*
 * $Id: directory.c,v 1.11 2003/07/15 08:26:37 germeier Exp $
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

#include <unistd.h>
#include <iconv.h>
#include <ctype.h>

#include "debug.h"
#include "io.h"
#include "mpio.h"
#include "directory.h"
#include <sys/time.h>

#define UNICODE "UNICODELITTLE"

/* the following function is copied from the linux kernel v2.4.18
 * file:/usr/src/linux/fs/fat/misc.c
 * it was written by Werner Almesberger and Igor Zhbanov
 * and is believed to be GPL
 */

/* Linear day numbers of the respective 1sts in non-leap years. */

static int day_n[] = { 0,31,59,90,120,151,181,212,243,273,304,334,0,0,0,0 };
		  /* JanFebMarApr May Jun Jul Aug Sep Oct Nov Dec */

/* Convert a MS-DOS time/date pair to a UNIX date (seconds since 1 1 70). */
int date_dos2unix(unsigned short,unsigned short);

int date_dos2unix(unsigned short time,unsigned short date)
{
	int month,year,secs;
	struct timezone sys_tz;
	struct timeval  tv;
	
	gettimeofday(&tv, &sys_tz);

	/* first subtract and mask after that... Otherwise, if
	   date == 0, bad things happen */
	month = ((date >> 5) - 1) & 15;
	year = date >> 9;
	secs = (time & 31)*2+60*((time >> 5) & 63)+(time >> 11)*3600+86400*
	    ((date & 31)-1+day_n[month]+(year/4)+year*365-((year & 3) == 0 &&
	    month < 2 ? 1 : 0)+3653);
			/* days since 1.1.70 plus 80's leap day */
	secs += sys_tz.tz_minuteswest*60;
	return secs;
}

/*
 * charset for filename encoding/converting
 */
BYTE *
mpio_charset_get(mpio_t *m)
{
  return strdup(m->charset);
}
  
BYTE   
mpio_charset_set(mpio_t *m, BYTE *charset)
{
  iconv_t ic;
  int     r = 1;
  
  ic = iconv_open(UNICODE, charset);
  if (ic == ((iconv_t)(-1)))
    r=0;
  iconv_close(ic);
  
  ic = iconv_open(charset, UNICODE);
  if (ic == ((iconv_t)(-1)))
    r=0;
  iconv_close(ic);

  if (r)
    {
      debugn(2, "setting new charset to: \"%s\"\n", charset);
      free(m->charset);
      m->charset=strdup(charset);
    } else {
      debugn(2, "could not set charset to: \"%s\"\n", charset);
    }
  
  return r;
}

int     
mpio_directory_init(mpio_t *m, mpio_mem_t mem, mpio_directory_t *dir,
		    WORD self, WORD parent)
{
  mpio_dir_entry_t *dentry;

  UNUSED(m);
  UNUSED(mem);
  
  memset(dir->dir,         0, BLOCK_SIZE);
  memset(dir->dir,      0x20, 11);
  memset(dir->dir+0x20, 0x20, 11);

  dentry = (mpio_dir_entry_t *)dir->dir;
  
  strncpy(dentry->name, ".       ", 8);
  strncpy(dentry->ext, "   ", 3);
  dentry->start[0] = self & 0xff;
  dentry->start[1] = self / 0x100;
  dentry->attr = 0x10;
  
  dentry++;
  strncpy(dentry->name, "..      ", 8);
  strncpy(dentry->ext, "   ", 3);
  dentry->start[0] = parent & 0xff;
  dentry->start[1] = parent / 0x100;
  dentry->attr = 0x10;

  hexdumpn(2, dir->dir, 64);

  return 0;
}


int     
mpio_directory_read(mpio_t *m, mpio_mem_t mem, mpio_directory_t *dir)
{
  mpio_fatentry_t *f = 0;
  
  f = mpio_dentry_get_startcluster(m, mem, dir->dentry);

  if (!f) 
    {      
      debug("something bad has happened here!");
      exit (-1);      
    }

  mpio_io_block_read(m, mem, f, dir->dir);

  hexdumpn(5, dir->dir, DIR_SIZE);	

  return 0;
}

BYTE
mpio_directory_is_empty(mpio_t *m, mpio_mem_t mem, mpio_directory_t *dir)
{
  mpio_dir_entry_t *dentry;
  BYTE r;

  UNUSED(m);
  UNUSED(mem);

  dentry = (mpio_dir_entry_t *)dir->dir;
  dentry += 2;

  r = MPIO_OK;
  if (dentry->name[0] != 0x00) 
    r = !r;
  
  return r;
}

  
int     
mpio_directory_write(mpio_t *m, mpio_mem_t mem, mpio_directory_t *dir)
{
  mpio_fatentry_t *f = 0;
  
  f = mpio_dentry_get_startcluster(m, mem, dir->dentry);
  if (!f) 
    {      
      debug("something bad has happened here!");
      exit (-1);      
    }

  if (mem==MPIO_INTERNAL_MEM) 
    {
      f->i_fat[0x01]= f->i_index;
      if (m->model >= MPIO_MODEL_FD100) 
	f->i_fat[0x0e] = f->i_index;	

      /* only one block needed for directory */
      f->i_fat[0x02]=0;
      f->i_fat[0x03]=1;

      /* set type to directory */
      f->i_fat[0x06] = FTYPE_ENTRY;

      hexdumpn(2, f->i_fat, 16);
    }
  
  mpio_io_block_delete(m, mem, f);
  mpio_io_block_write(m, mem, f, dir->dir);

  return 0;
}

/* directory operations */
BYTE *
mpio_directory_open(mpio_t *m, mpio_mem_t mem)
{  
  BYTE *out;  
  if (mem == MPIO_EXTERNAL_MEM) {
    if (m->external.id)  { 
      out = m->external.cdir->dir;
    } else {
      return NULL;
    }
  } else {    
    out = m->internal.cdir->dir;
  }

  if (out[0] == 0x00) 
    {
      debugn(3, "directory is empty\n");
      return NULL;
    }
  
  debugn(3, "first dentry: %08x\n", out);
  
  return out;  
}

int     
mpio_directory_make(mpio_t *m, mpio_mem_t mem, BYTE *dir)
{
  mpio_smartmedia_t *sm;
  mpio_directory_t *new;
  mpio_fatentry_t   *f, *current;
  WORD self, parent;
  struct tm tt;
  time_t curr;

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  if ((strcmp(dir, "..") == 0) ||
      (strcmp(dir, ".") == 0))
    {
      debugn(2, "directory name not allowed: %s\n", dir);
      return MPIO_ERR_DIR_NAME_ERROR;    
    }

  /* find free sector */
  f = mpio_fatentry_find_free(m, mem, FTYPE_ENTRY);
  if (!f) 
    {
      debug("could not free cluster for file!\n");
      return (MPIO_ERR_FAT_ERROR);
    } else {
      self=f->entry;
    }  

  /* find file-id for internal memory */
  if (mem==MPIO_INTERNAL_MEM) 
    {      
      f->i_index=mpio_fat_internal_find_fileindex(m);
      debugn(2, "fileindex: %02x\n", f->i_index);
      f->i_fat[0x01]= f->i_index;
      if (m->model >= MPIO_MODEL_FD100) 
	f->i_fat[0x0e] = f->i_index;	
      self         = f->i_index;

      /* only one block needed for directory */
      f->i_fat[0x02]=0;
      f->i_fat[0x03]=1;
      hexdumpn(2, f->i_fat, 16);
    }

  if (sm->cdir == sm->root) 
    {
      parent=0;
    } else {
      current = mpio_dentry_get_startcluster(m, mem, sm->cdir->dentry);
      if (!current) {
	debugn(2, "error creating directory");
	return MPIO_ERR_FAT_ERROR;
      }

      if (mem==MPIO_INTERNAL_MEM)
	{	  
	  parent = current->i_index;
	} else {
	  parent = current->entry;
	}
    }


  new = malloc(sizeof(mpio_directory_t));
  mpio_directory_init(m, mem, new, self, parent);  

  mpio_fatentry_set_eof(m ,mem, f);
  mpio_io_block_write(m, mem, f, new->dir);
  time(&curr);
  tt = * localtime(&curr);
  mpio_dentry_put(m, mem,
		  dir, strlen(dir),
		      mktime(&tt), 
		      0, self, 0x10);
  
  free(new);
  
  return MPIO_OK;
}

int     
mpio_directory_cd(mpio_t *m, mpio_mem_t mem, BYTE *dir)
{
  mpio_smartmedia_t *sm;
  BYTE *p;
  BYTE month, day, hour, minute, type;
  BYTE fname[100];
  WORD year;  
  DWORD fsize;
  int size;
  BYTE pwd[INFO_LINE];
  mpio_directory_t *old, *new;

  if (strcmp(dir, ".")==0)
    return MPIO_OK;

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  if (strcmp(dir, "..") == 0)
    {
      if (sm->cdir->prev)
	{	  
	  old            = sm->cdir;
	  sm->cdir       = sm->cdir->prev;
	  sm->cdir->next = NULL;
	  free(old);
	}

      return MPIO_OK;
    }

  mpio_directory_pwd(m, mem, pwd);

  if ((strlen(pwd) + strlen(dir) + 2) > INFO_LINE)
  {
      debugn(2, "directory name gets to long!\n");
      return MPIO_ERR_DIR_TOO_LONG;    
  }


  p = mpio_dentry_find_name(m, mem, dir);
  
  /* second try */
  if (!p)
    p = mpio_dentry_find_name_8_3(m, mem, dir);

  if (!p) 
    {
      debugn(2, "could not find directory: %s\n", dir);
      return MPIO_ERR_DIR_NOT_FOUND;    
    } 

  mpio_dentry_get(m, mem, p,
		  fname, 100,
		  &year, &month, &day,
		  &hour, &minute, &fsize,
		  &type);

  if (type != FTYPE_DIR)
    {
      debugn(2, "this is not a directory: %s\n", dir);
      return MPIO_ERR_DIR_NOT_A_DIR;    
    }

  new             = malloc(sizeof(mpio_directory_t));
  strcpy(new->name, dir);		   
  new->next       = NULL;
  new->prev       = sm->cdir;
  new->dentry     = p;
  sm->cdir->next  = new;
  sm->cdir        = new;

  mpio_directory_pwd(m, mem, pwd);

  if (strcmp(dir, "/") != 0)
    {
      /* read new directory */
      size=mpio_directory_read(m, mem, sm->cdir);      
    }

  return MPIO_OK;
  
}


void    
mpio_directory_pwd(mpio_t *m, mpio_mem_t mem, BYTE pwd[INFO_LINE])
{
  mpio_smartmedia_t *sm;  
  mpio_directory_t  *d;

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  d = sm->root->next;
  pwd[0] = 0;

  if (!d)
    strcat(pwd, "/");

  while (d) 
    {
      strcat(pwd, "/");
      debugn(2, "name: %s\n", d->name);
      strcat(pwd, d->name);
      d = d->next;
    }    

  return;
}

mpio_dir_entry_t *
mpio_dentry_filename_write(mpio_t *m, mpio_mem_t mem, BYTE *p, 
			   BYTE *filename, int filename_size)
{
  BYTE *unicode = 0;
  BYTE *back, *fback;
  BYTE *fname = 0;
  iconv_t ic;
  int in = 0, out = 0;
  int fin = 0, fout = 0;
  int count = 0;
  BYTE index;
  BYTE f_8_3[13];
  BYTE alias_check;
  mpio_dir_slot_t  *slot;
  mpio_dir_entry_t *dentry;
  DWORD i, j;
  int points;
  
  /* generate vfat filename in UNICODE */
  ic = iconv_open(UNICODE, m->charset);
  fin = in = filename_size + 1;
  fout = out = filename_size * 2 + 2 + 26;
  fname = malloc(in);
  fback = fname;
  unicode = malloc(out);
  back = unicode;

  memset(fname, 0, in);
  snprintf(fname, in, "%s", filename);
  memset(unicode, 0xff, out);
  iconv(ic, (char **)&fback, &fin, (char **)&back, &fout);
  iconv_close(ic);
  hexdump(fname, in);
  hexdump(unicode, out);

  back = unicode;
  
  count = filename_size / 13;
  if (filename_size % 13)
    count++;

  /* find uniq 8.3 filename */
  memset(f_8_3, 0x20, 12);
  f_8_3[8]='.';
  f_8_3[12]=0x00;

  i=0;
  points=0;
  /* count points to later find the correct file extension */
  while (i<(strlen(filename))) 
    {
      if (filename[i] == '.')
	points++;
      i++;
    }

  /* if we do not find any points we set the value ridiculously high,
     then everything falls into place */
  if (!points)
    points=1024*1024;

  i=j=0;
  while ((j<8) && (points) && (i<(strlen(filename))))
    {
      if (filename[i] == '.') 
	{
	  points--;
	} else {
	  if (filename[i]!=' ') {
	    f_8_3[j] = toupper(filename[i]);
	    j++;
	  }
	}
      i++;
    }

  j=i;
  while((points) && (j<(strlen(filename))))
    {
      if (filename[j] == '.') 
	points--;
      j++;
    }  
  
  i=9;
  while ((i<12) && (j<(strlen(filename))))
    {
      f_8_3[i] = toupper(filename[j]);
      i++;
      j++;
    }

  if(mpio_dentry_find_name_8_3(m, mem, f_8_3)) {
    f_8_3[6]='~';
    f_8_3[7]='0';
  }

  while(mpio_dentry_find_name_8_3(m, mem, f_8_3))
    f_8_3[7]++;
  
  hexdumpn(5, f_8_3, 13);

  /* calculate checksum for 8.3 alias */
  alias_check = 0;
  for (i = 0; i < 12; i++) {
    if (i!=8) /* yuck */
      alias_check = (((alias_check & 0x01)<<7)|
		     ((alias_check & 0xfe)>>1)) + f_8_3[i];
  }

  slot = (mpio_dir_slot_t *)p;

  index = 0x40 + count;
  while (count > 0) {
    mpio_dentry_copy_to_slot(back + ((count - 1) * 26), slot);
    hexdump((char *)back + ((count - 1) * 26), 0x20);
    slot->id = index;
    slot->attr = 0x0f;
    slot->reserved = 0x00;
    slot->start[0] = 0x00;
    slot->start[1] = 0x00;
    slot->alias_checksum = alias_check;   // checksum for 8.3 alias 

    hexdumpn(5, (char *)slot, 0x20);
    
    slot++;
    count--;
    index = count;
  }

  dentry = (mpio_dir_entry_t *)slot;

  memcpy(dentry->name, f_8_3, 8);
  memcpy(dentry->ext, f_8_3+9, 3);

  hexdumpn(5, (char *)dentry, 0x20);

  free(unicode);
  free(fname);

  return dentry;
}


int
mpio_dentry_get_size(mpio_t *m, mpio_mem_t mem, BYTE *buffer)
{
  mpio_dir_entry_t *dentry;

  UNUSED(mem);

  if (!buffer)
    return -1;  

  UNUSED(m);
  
  dentry = (mpio_dir_entry_t *)buffer;

  if ((dentry->name[0] & 0x40)    && 
      (dentry->attr == 0x0f)      &&
      (dentry->start[0] == 0x00)  &&
      (dentry->start[1] == 0x00)) {
    dentry++;    
    while ((dentry->attr == 0x0f)      &&
	   (dentry->start[0] == 0x00)  &&
	   (dentry->start[1] == 0x00)) {
      /* this/these are vfat slots */
      dentry++;
    }
  }  
  dentry++;
  
  return(((BYTE *)dentry) - buffer);
}  

BYTE*
mpio_dentry_next(mpio_t *m, mpio_mem_t mem, BYTE *buffer)
{
  int size;
  BYTE *out;
  
  size = mpio_dentry_get_size(m, mem, buffer);
  
  if (size<=0)
    return NULL;
  
  out = buffer + size;
  
  if (*out == 0x00) 
    {
      debugn(3, "no more entries\n");
      return NULL;  
    }
  
  debugn(3, "next  dentry: %08x\n", out);
  
  return out;
}      

int 
mpio_dentry_get_raw(mpio_t *m, mpio_mem_t mem, BYTE *dentry, 
		    BYTE *buffer, int bufsize)
{
  int size;
  
  size = mpio_dentry_get_size(m, mem, buffer);
  debugn(3, "dentry size is: 0x%02x\n", size);
  
  if (size < 0)
    return size;
  
  if (size > bufsize)
    return -2;
  
  memcpy(buffer, dentry, size);
  
  return size;
}

void
mpio_dentry_copy_from_slot(BYTE *buffer, mpio_dir_slot_t *slot)
{
  memcpy(buffer, slot->name0_4, 10);
  memcpy(buffer + 10, slot->name5_10, 12);
  memcpy(buffer + 22, slot->name11_12, 4);
}

void
mpio_dentry_copy_to_slot(BYTE *buffer, mpio_dir_slot_t *slot)
{
  memcpy(slot->name0_4, buffer, 10);
  memcpy(slot->name5_10, buffer + 10, 12);
  memcpy(slot->name11_12, buffer + 22, 4);
}

int
mpio_dentry_get(mpio_t *m, mpio_mem_t mem, BYTE *buffer,                   
		BYTE *filename, int filename_size,
		WORD *year, BYTE *month, BYTE *day,
		BYTE *hour, BYTE *minute, DWORD *fsize, BYTE *type)
{
  BYTE filename_8_3[13];
  
  return mpio_dentry_get_real(m, mem, buffer, filename, filename_size, 
			      filename_8_3,
			      year, month, day, hour, minute, fsize, type);
}
  
/* TODO: please clean me up !!! */
int
mpio_dentry_get_real(mpio_t *m, mpio_mem_t mem, BYTE *buffer,                   
		     BYTE *filename, int filename_size,
		     BYTE *filename_8_3,
		     WORD *year, BYTE *month, BYTE *day,
		     BYTE *hour, BYTE *minute, DWORD *fsize,
		     BYTE *type)
{
  int date, time;  
  int vfat = 0;  
  int num_slots = 0;  
  int slots = 0;
  int in = 0, out = 0, iconv_return;
  mpio_dir_entry_t *dentry;
  mpio_fatentry_t  *f;
  mpio_dir_slot_t  *slot;
  BYTE *unicode = 0;
  BYTE *uc;
  BYTE *fname = 0;
  iconv_t ic;
  int dsize, i;
  
  if (buffer == NULL)
    return -1;  

  dentry = (mpio_dir_entry_t *)buffer;

  if ((dentry->name[0] & 0x40)    && 
      (dentry->attr == 0x0f)      &&
      (dentry->start[0] == 0x00)  &&
      (dentry->start[1] == 0x00)) 
    {
      dsize = mpio_dentry_get_size(m, mem, buffer);
      debugn(3, "dentry size is: 0x%02x\n", dsize);
      hexdump(buffer, dsize);
      num_slots = (dsize / 0x20) - 1;
      slots = num_slots - 1;
      dentry++;
      vfat++;    
      in = num_slots * 26;
      out = num_slots * 13;
      unicode = malloc(in + 2);
      memset(unicode, 0x00, (in+2));
      uc = unicode;
      fname = filename;
      slot = (mpio_dir_slot_t *)buffer;
      mpio_dentry_copy_from_slot(unicode + (26 * slots), slot);
      slots--;
      
      while ((dentry->attr == 0x0f)      &&
	     (dentry->start[0] == 0x00)  &&
	     (dentry->start[1] == 0x00)) 
	{
	  /* this/these are vfat slots */
	  slot = (mpio_dir_slot_t *)dentry;
	  mpio_dentry_copy_from_slot((unicode + (26 * slots)), slot);
	  dentry++;
	  slots--;
	}
    }  
  
  if (vfat) 
    {
      ic = iconv_open(m->charset, UNICODE);
      memset(fname, 0, filename_size);
      hexdumpn(4, unicode, in+2);
      debugn(4, "before iconv: in: %2d - out: %2d\n", in, out);
      iconv_return = iconv(ic, (char **)&uc, &in, (char **)&fname, &out);
      debugn(4, "after  iconv: in: %2d - out: %2d (return: %d)\n", in, out,
	     iconv_return);
      hexdumpn(4, filename, (num_slots*13)-out);
      iconv_close(ic);
    } 
  free(unicode);

  memcpy(filename_8_3, dentry->name, 8);
  i=8;
  while(filename_8_3[i-1]==' ')
    i--;
  filename_8_3[i++] = '.';	
  memcpy(filename_8_3 + i, dentry->ext, 3);
  i+=3;
  while(filename_8_3[i-1]==' ')
    i--;  
  filename_8_3[i] = 0;
  hexdumpn(4, filename_8_3, 13);

  if (!vfat) 
    {    
      if (filename_size >= 12) 
	{
	  snprintf(filename, 13, "%s", filename_8_3);	    
	  /* UGLY !! */      
	  if (((strncmp(dentry->name, ".       ", 8)==0) &&
	       (strncmp(dentry->ext, "   ", 3) == 0)))
	    filename[1]=0;
	  if (((strncmp(dentry->name, "..      ", 8)==0) &&
	       (strncmp(dentry->ext, "   ", 3) == 0)))
	    filename[2]=0;
	} else {
	  snprintf(filename, filename_size, "%s", "ERROR");
	}
    }

  date  = (dentry->date[1] * 0x100) + dentry->date[0];
  *year  = date / 512 + 1980;
  *month = (date / 32) & 0xf;
  *day   = date & 0x1f;
  
  time  = (dentry->time[1] * 0x100) + dentry->time[0];
  *hour  = time / 2048;
  *minute= (time / 32) & 0x3f;

  *fsize = dentry->size[3];
  *fsize *= 0x100;
  *fsize += dentry->size[2];
  *fsize *= 0x100;
  *fsize += dentry->size[1];
  *fsize *= 0x100;
  *fsize += dentry->size[0];

  if (dentry->attr & 0x10) {
    /* is this a directory? */
    *type = FTYPE_DIR;
  } else {
    *type = FTYPE_PLAIN;
    if (mem == MPIO_INTERNAL_MEM) {
      f = mpio_dentry_get_startcluster(m, mem, buffer);
      if (f) {	
	*type = m->internal.fat[f->entry * 0x10 + 0x06];
      } else {
	/* we did not find the startcluster, thus the internal 
	   FAT is broken for this file */
	*type = FTYPE_BROKEN;
      }
    }    
  }

  return(((BYTE *)dentry) - buffer);
}

/* read "size" sectors of fat into the provided buffer */
int
mpio_rootdir_read (mpio_t *m, mpio_mem_t mem)
{
  mpio_smartmedia_t *sm;  
  BYTE recvbuff[SECTOR_SIZE];
  int i;

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;
  
  for (i = 0; i < DIR_NUM; i++) {
    if (mpio_io_sector_read(m, mem, (sm->dir_offset + i), recvbuff))
      return 1;

    memcpy(sm->root->dir + (i * SECTOR_SIZE), recvbuff, SECTOR_SIZE);
  }

  return (0);
}

int
mpio_rootdir_clear (mpio_t *m, mpio_mem_t mem)
{
  mpio_smartmedia_t *sm;  

  if (mem == MPIO_INTERNAL_MEM) sm=&m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm=&m->external;

  memset(sm->root->dir, 0x00, DIR_SIZE);

  return 0;
}

BYTE    
mpio_dentry_is_dir(mpio_t *m, mpio_mem_t mem, BYTE *p)
{
  int s;
  mpio_dir_entry_t *dentry;
  BYTE r;

  s  = mpio_dentry_get_size(m, mem, p);
  s -= DIR_ENTRY_SIZE ;

  dentry = (mpio_dir_entry_t *)p;
  
  while (s != 0) {
    dentry++;
    s -= DIR_ENTRY_SIZE ;
  }

  if (dentry->attr & 0x10) {
    r = MPIO_OK;
  } else {
    r= !MPIO_OK;
  }

  return r;
}

int
mpio_dentry_get_filesize(mpio_t *m, mpio_mem_t mem, BYTE *p)
{
  int s;
  int fsize;
  mpio_dir_entry_t *dentry;

  s  = mpio_dentry_get_size(m, mem, p);
  s -= DIR_ENTRY_SIZE ;

  dentry = (mpio_dir_entry_t *)p;

  while (s != 0) {
    dentry++;
    s -= DIR_ENTRY_SIZE ;
  }

  if (dentry->attr & 0x10) {
    fsize = BLOCK_SIZE;
  } else {
    fsize  = dentry->size[3];
    fsize *= 0x100;
    fsize += dentry->size[2];
    fsize *= 0x100;
    fsize += dentry->size[1];
    fsize *= 0x100;
    fsize += dentry->size[0];
  }
  
  return fsize; 
}

long
mpio_dentry_get_time(mpio_t *m, mpio_mem_t mem, BYTE *p)
{
  int s;
  mpio_dir_entry_t *dentry;

  s  = mpio_dentry_get_size(m, mem, p);
  s -= DIR_ENTRY_SIZE ;

  dentry = (mpio_dir_entry_t *)p;

  while (s != 0) {
    dentry++;
    s -= DIR_ENTRY_SIZE ;
  }


  return date_dos2unix((dentry->time[0]+dentry->time[1]*0x100),
		       (dentry->date[0]+dentry->date[1]*0x100));
}



mpio_fatentry_t *
mpio_dentry_get_startcluster(mpio_t *m, mpio_mem_t mem, BYTE *p)
{
  int s;
  int cluster;
  BYTE i_index;
  mpio_dir_slot_t *dentry;
  mpio_fatentry_t *new;

  s  = mpio_dentry_get_size(m, mem, p);
  s -= DIR_ENTRY_SIZE ;

  dentry = (mpio_dir_slot_t *)p;

  while (s != 0) {
    dentry++;
    s -= DIR_ENTRY_SIZE ;
  }

  cluster = dentry->start[1] * 0x100 + dentry->start[0];

  if (mem == MPIO_INTERNAL_MEM) 
    {
      i_index=dentry->start[0];
      cluster = mpio_fat_internal_find_startsector(m, cluster);
      if (cluster < 0)
	return NULL;
    }

  new = mpio_fatentry_new(m, mem, cluster, FTYPE_MUSIC);

  if (mem == MPIO_INTERNAL_MEM) 
    { 
      new->entry=cluster;
      new->i_index=i_index;
      mpio_fatentry_entry2hw(m, new);      
    }  

  debugn(2,"i_index=0x%02x\n", new->i_index);
  
  return new; 
}

int
mpio_dentry_put(mpio_t *m, mpio_mem_t mem,
		BYTE *filename, int filename_size,
		time_t date, DWORD fsize, WORD ssector, BYTE attr)
{
  BYTE *p;
  mpio_dir_entry_t *dentry;
  
  /* read and copied code from mtools-3.9.8/directory.c 
   * to make this one right 
   */
  struct tm *now;
  time_t date2 = date;
  unsigned char hour, min_hi, min_low, sec;
  unsigned char year, month_hi, month_low, day;


  p = mpio_directory_open(m, mem);
  if (p) {
    while (*p != 0x00)
      p += 0x20;
  } else {
    if (mem == MPIO_EXTERNAL_MEM) 
      p = m->external.cdir->dir;
    if (mem == MPIO_INTERNAL_MEM) 
      p = m->internal.cdir->dir;
  }

  dentry = mpio_dentry_filename_write(m, mem, p, filename, filename_size);

  dentry->attr = attr;
  dentry->lcase = 0x00;

  /* read and copied code from mtools-3.9.8/directory.c 
   * to make this one right 
   */
  now = localtime(&date2);
  dentry->ctime_ms = 0;
  hour = now->tm_hour << 3;
  min_hi = now->tm_min >> 3;
  min_low = now->tm_min << 5;
  sec = now->tm_sec / 2;
  dentry->ctime[1] = dentry->time[1] = hour + min_hi;
  dentry->ctime[0] = dentry->time[0] = min_low + sec;
  year = (now->tm_year - 80) << 1;
  month_hi = (now->tm_mon + 1) >> 3;
  month_low = (now->tm_mon + 1) << 5;
  day = now->tm_mday;
  dentry -> adate[1] = dentry->cdate[1] = dentry->date[1] = year + month_hi;
  dentry -> adate[0] = dentry->cdate[0] = dentry->date[0] = month_low + day;
  
  dentry->size[0] = fsize & 0xff;
  dentry->size[1] = (fsize / 0x100) & 0xff;
  dentry->size[2] = (fsize / 0x10000) & 0xff;
  dentry->size[3] = (fsize / 0x1000000) & 0xff;
  dentry->start[0] = ssector & 0xff;
  dentry->start[1] = ssector / 0x100;

  /* what do we want to return? */
  return 0;
}

BYTE *
mpio_dentry_find_name_8_3(mpio_t *m, BYTE mem, BYTE *filename)
{
  BYTE *p;
  BYTE bdummy;
  WORD wdummy;
  BYTE fname[129];
  BYTE fname_8_3[13];
  DWORD ddummy;
  BYTE *found = 0;

  p = mpio_directory_open(m, mem);
  while ((p) && (!found)) {
    mpio_dentry_get_real (m, mem, p,
			  fname, 128,
			  fname_8_3,
			  &wdummy, &bdummy, &bdummy,
			  &bdummy, &bdummy, &ddummy, &bdummy);
    if ((strcmp(fname_8_3, filename) == 0) &&
	(strcmp(filename,fname_8_3) == 0)) {
      found = p;
      p = NULL;
    }
    
    p = mpio_dentry_next(m, mem, p);
  }
	 
  return found;
}

BYTE *
mpio_dentry_find_name(mpio_t *m, BYTE mem, BYTE *filename)
{
  BYTE *p;
  BYTE bdummy;
  WORD wdummy;
  BYTE fname[129];
  DWORD ddummy;
  BYTE *found = 0;
  
  p = mpio_directory_open(m, mem);
  while ((p) && (!found)) {
    mpio_dentry_get (m, mem, p,
		     fname, 128,
		     &wdummy, &bdummy, &bdummy,
		     &bdummy, &bdummy, &ddummy, &bdummy);
    if ((strcmp(fname,filename) == 0) && (strcmp(filename,fname) == 0)) {
      found = p;
      p = NULL;
    }
    
    p = mpio_dentry_next(m, mem, p);
  }
	 
  return found;
}


int	
mpio_dentry_delete(mpio_t *m, BYTE mem, BYTE *filename)
{
  mpio_smartmedia_t *sm;
  BYTE *start;
  int  size;
  BYTE tmp[DIR_SIZE];

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  start = mpio_dentry_find_name(m, mem, filename);
  
  /* second try */
  if (!start)
    start = mpio_dentry_find_name_8_3(m, mem, filename);

  if (!start) {
    debugn(2, "could not find file: %s\n", filename);
    return 0;    
  } 

  size = mpio_dentry_get_size(m, mem, start);
  
  if (size <= 0) {
    debug("fatal error in mpio_dentry_delete\n");
    return 0;    
  }

  debugn(5, "size: %2x\n", size);

  /* clear new buffer */
  memset(tmp, 0, DIR_SIZE);
  /* copy before delete */
  if (start != sm->cdir->dir)
    memcpy(tmp, sm->cdir->dir, (start - (sm->cdir->dir)));
  /* copy after delete */
  memcpy(tmp + (start - (sm->cdir->dir)), (start + size),
	 (sm->cdir->dir + DIR_SIZE - (start + size)));

  memcpy(sm->cdir->dir, tmp, DIR_SIZE);
  
  return 0;
}

void 
mpio_dentry_move(mpio_t *m,mpio_mem_t mem,BYTE *m_file,BYTE *a_file) {
  mpio_smartmedia_t *sm;

  BYTE *t0,*t1,*t2,*t3;
  
  int  s0,s1,s2,s3;
  int  m_file_s,a_file_s;
  BYTE tmp[DIR_SIZE];
  BYTE *b_file;

  if (mem == MPIO_INTERNAL_MEM) 
    sm = &m->internal;

  if (mem == MPIO_EXTERNAL_MEM) 
    sm = &m->external;

  if (m_file == a_file)
    return;

  m_file_s = mpio_dentry_get_size(m, mem, m_file);
  a_file_s = mpio_dentry_get_size(m, mem, a_file);

  // -- we determine the 'befor' file. The start of the region which needs to be moved down
  // -- if a_file == NULL this is the start of the directory.

  if(a_file==NULL) {
    b_file = sm->cdir->dir;
  } else {
    b_file = a_file + a_file_s;
  }
  
  if(b_file == m_file) {
    return;
  }

  /** We disect the whole directory in four pieces.
      in different ways according to the direction 
      the directoy entry is moved (up or down).
  */

  if(m_file > b_file) {
    fprintf(stderr,"U ");
    t0 = sm->cdir->dir;
    s0 = b_file - sm->cdir->dir;

    t1 = m_file;
    s1 = m_file_s;
    
    t2 = b_file;
    s2 = m_file - b_file;

    t3 = m_file + m_file_s;
    s3 = DIR_SIZE - (m_file-sm->cdir->dir) - m_file_s;
  } else { 
    fprintf(stderr,"D ");
    t0 = sm->cdir->dir;
    s0 = m_file - sm->cdir->dir;
    
    t1 = m_file + m_file_s;
    s1 = a_file + a_file_s - (m_file + m_file_s);

    t2 = m_file;
    s2 = m_file_s;

    t3 = b_file;
    s3 = DIR_SIZE - (b_file - sm->cdir->dir);
  }

  if(s0) {
    memcpy(tmp,t0,s0);
  }

  if(s1) {
    memcpy(tmp + s0 , t1 , s1 );
  }

  if(s2) {
    memcpy(tmp + s0 + s1, t2, s2);
  }

  if(s3) {
    memcpy(tmp + s0 + s1 + s2, t3, s3);
  }
  
  fprintf(stderr," -- t0=%ld, s0=%d, t1=%ld, s1=%d, t2=%ld, s2=%d, t3=%ld, s3=%d; sum=%d, DIRSIZE=%d\n",
	  (long)t0,s0,(long)t1,s1,(long)t2,s2,(long)t3,s3,s0+s1+s2+s3,DIR_SIZE);

  memcpy(sm->cdir->dir, tmp, DIR_SIZE);
}

void    
mpio_dentry_switch(mpio_t *m, mpio_mem_t mem, BYTE *file1, BYTE *file2)
{
  mpio_smartmedia_t *sm;
  BYTE *p1, *p2;
  BYTE *current;
  int size1 , size2;
  BYTE tmp[DIR_SIZE];

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  if (file1 == file2)
    return;

  if (file1<file2)
    {
      p1 = file1;
      p2 = file2;
    } else {
      p1 = file2;
      p2 = file1;
    } 
  size1 = mpio_dentry_get_size(m, mem, p1);
  size2 = mpio_dentry_get_size(m, mem, p2);

  current = tmp;
  memset(tmp, 0xff, DIR_SIZE);
  /* before the first file */
  if (p1 != sm->cdir->dir)
    {      
      memcpy(current, sm->cdir->dir, p1 - sm->cdir->dir);
      current += (p1 - sm->cdir->dir);
    }  
  /* the second file*/
  memcpy(current, p2, size2);
  current += size2;
  /* between the files */
  memcpy(current, p1+size1, (p2-p1-size1));
  current += (p2-p1-size1);  
  /* the first file */
  memcpy(current, p1, size1);
  current += size1;
  /* and the rest */
  memcpy(current, p2+size2, (sm->cdir->dir+DIR_SIZE-p2-size2));

  /* really update the directory */
  memcpy(sm->cdir->dir, tmp, DIR_SIZE);

  return;
}

void    
mpio_dentry_rename(mpio_t *m, mpio_mem_t mem, BYTE *p, BYTE *newfilename)
{
  mpio_smartmedia_t *sm;
  BYTE *current;
  int size1 , size2, offset, offset_d1, offset_d2;
  BYTE tmp[DIR_SIZE];

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;
  
  current = sm->cdir->dir;

  size1 = mpio_dentry_get_size(m, mem, p) / 0x20;
  size2 = (strlen(newfilename) / 13) + 1;
  if ((strlen(newfilename) % 13))
    size2++;

  debugn(2, "size1: %d   size2: %d\n", size1, size2);

  /* we want to copy the last dentry to the right location
   * so we avoid reading and writing the same information
   */
  size1--; /* kludge so we can do compares without worry */
  size2--;

  memcpy(tmp, current, DIR_SIZE); 
  offset   = p - current;
  offset_d1 = offset + (size1 * 0x20);
  offset_d2 = offset + (size2 * 0x20);

  if (size2>size1) {
    /* new filename needs at least one slot more than the old one */
    memcpy(current+offset_d2, tmp+offset_d1, (DIR_SIZE-offset_d1));
  }

  if (size2<size1) {
    /* new filename needs at least one slot less than the old one */
    memset(p+offset, 0, (DIR_SIZE-offset)); /* clear to avoid bogus dentries */
    memcpy(current+offset_d2, tmp+offset_d1, (DIR_SIZE-offset_d2));
  }
    
  mpio_dentry_filename_write(m, mem, p, newfilename, strlen(newfilename));

  return ;  
}
