/* 
 *
 * $Id: directory.c,v 1.4 2002/09/08 23:22:48 germeier Exp $
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

#include <unistd.h>
#include <iconv.h>

#include "debug.h"
#include "io.h"
#include "mpio.h"
#include "directory.h"

/* directory operations */
BYTE *
mpio_directory_open(mpio_t *m, BYTE mem)
{  
  BYTE *out;  
  if (mem == MPIO_EXTERNAL_MEM) {
    if (m->external.id)  { 
      out = m->external.dir;
    } else {
      return NULL;
    }
  } else {    
    out = m->internal.dir;
  }

  if (out[0] == 0x00)
    return NULL;
  
  return out;  
}

int
mpio_dentry_get_size(mpio_t *m, BYTE *buffer)
{
  mpio_dir_entry_t *dentry;

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
mpio_dentry_next(mpio_t *m, BYTE *buffer)
{
  int size;
  BYTE *out;
  
  size = mpio_dentry_get_size(m, buffer);
  if (size<0)
    return NULL;
  
  out = buffer + size;
  
  if (*out == 0x00)
    return NULL;  
  
  return out;
}      

int 
mpio_dentry_get_raw(mpio_t *m, BYTE *dentry, BYTE *buffer, int bufsize)
{
  int size;
  
  size = mpio_dentry_get_size(m, buffer);
  
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
mpio_dentry_get(mpio_t *m, BYTE *buffer,                   
		BYTE *filename, int filename_size,
		WORD *year, BYTE *month, BYTE *day,
		BYTE *hour, BYTE *minute, DWORD *fsize)
{
  BYTE filename_8_3[12];
  
  return mpio_dentry_get_real(m, buffer, filename, filename_size, 
			      filename_8_3,
			      year, month, day, hour, minute, fsize);
}
  
/* TODO: please clean me up !!! */
int
mpio_dentry_get_real(mpio_t *m, BYTE *buffer,                   
		     BYTE *filename, int filename_size,
		     BYTE filename_8_3[12],
		     WORD *year, BYTE *month, BYTE *day,
		     BYTE *hour, BYTE *minute, DWORD *fsize)
{
  int date, time;  
  int vfat = 0;  
  int num_slots = 0;  
  int slots = 0;
  int in = 0, out = 0;
  mpio_dir_entry_t *dentry;
  mpio_dir_slot_t  *slot;
  BYTE *unicode = 0;
  BYTE *uc;
  BYTE *fname = 0;
  iconv_t ic;
  
  if (buffer == NULL)
    return -1;  

  dentry = (mpio_dir_entry_t *)buffer;

  if ((dentry->name[0] & 0x40)    && 
      (dentry->attr == 0x0f)      &&
      (dentry->start[0] == 0x00)  &&
      (dentry->start[1] == 0x00)) {
    num_slots = (mpio_dentry_get_size(m, buffer) / 0x20) - 1;
    slots = num_slots - 1;
    dentry++;
    vfat++;    
    unicode = malloc(256);
    uc = unicode;
    fname = filename;
    slot = (mpio_dir_slot_t *)buffer;
    mpio_dentry_copy_from_slot(unicode + (26 * slots), slot);
    slots--;

    while ((dentry->attr == 0x0f)      &&
	   (dentry->start[0] == 0x00)  &&
	   (dentry->start[1] == 0x00)) {
      /* this/these are vfat slots */
      slot = (mpio_dir_slot_t *)dentry;
      mpio_dentry_copy_from_slot((unicode + (26 * slots)), slot);
      dentry++;
      slots--;
    }
  }  

  if (vfat) {
    ic = iconv_open("ASCII", "UNICODE");
    in = num_slots * 26;
    out = num_slots * 13;
    memset(fname, 0, filename_size);
    iconv(ic, (char **)&uc, &in, (char **)&fname, &out);
    iconv_close(ic);
    free(unicode);
  } else {
    memcpy(filename_8_3, dentry->name, 8);
    filename_8_3[0x08] = '.';	
    memcpy(filename_8_3 + 0x09, dentry->ext, 3);
    filename_8_3[0x0c] = 0;

    if (filename_size >= 12) {
      snprintf(filename, 13, "%s", filename_8_3);	    
    } else {
      snprintf(filename, 12, "%s", "ERROR");
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

    memcpy(sm->dir + (i * SECTOR_SIZE), recvbuff, SECTOR_SIZE);
  }

  return (0);
}

int
mpio_rootdir_clear (mpio_t *m, mpio_mem_t mem)
{
  mpio_smartmedia_t *sm;  

  if (mem == MPIO_INTERNAL_MEM) sm=&m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm=&m->external;

  memset(sm->dir, 0x00, DIR_SIZE);

  return 0;
}

int
mpio_dentry_get_filesize(mpio_t *m, mpio_mem_t mem, BYTE *p)
{
  int s;
  int fsize;
  mpio_dir_entry_t *dentry;

  s  = mpio_dentry_get_size(m, p);
  s -= DIR_ENTRY_SIZE ;

  dentry = (mpio_dir_entry_t *)p;

  while (s != 0) {
    dentry++;
    s -= DIR_ENTRY_SIZE ;
  }

  fsize  = dentry->size[3];
  fsize *= 0x100;
  fsize += dentry->size[2];
  fsize *= 0x100;
  fsize += dentry->size[1];
  fsize *= 0x100;
  fsize += dentry->size[0];

  return fsize; 
}


mpio_fatentry_t *
mpio_dentry_get_startcluster(mpio_t *m, mpio_mem_t mem, BYTE *p)
{
  int s;
  DWORD cluster;
  mpio_dir_slot_t *dentry;
  mpio_fatentry_t *new;

  s  = mpio_dentry_get_size(m, p);
  s -= DIR_ENTRY_SIZE ;

  dentry = (mpio_dir_slot_t *)p;

  while (s != 0) {
    dentry++;
    s -= DIR_ENTRY_SIZE ;
  }

  cluster = dentry->start[1] * 0x100 + dentry->start[0];

  if (mem == MPIO_INTERNAL_MEM) 
    cluster = mpio_fat_internal_find_startsector(m, cluster);
  if (cluster < 0)
    return NULL;

  new = mpio_fatentry_new(m, mem, cluster);

  if (mem == MPIO_INTERNAL_MEM) 
    { 
      new->entry=cluster;
      mpio_fatentry_entry2hw(m, new);      
    }  
  
  return new; 
}

int
mpio_dentry_put(mpio_t *m, BYTE mem,
		BYTE *filename, int filename_size,
		WORD year, BYTE month, BYTE day,
		BYTE hour, BYTE minute, DWORD fsize, WORD ssector)
{
  BYTE *unicode = 0;
  BYTE *back, *fback;
  BYTE *fname = 0;
  iconv_t ic;
  int in = 0, out = 0;
  int fin = 0, fout = 0;
  int count = 0;
  BYTE index;
  
  BYTE *p;
  mpio_dir_entry_t *dentry;
  mpio_dir_slot_t  *slot;
  
  p = mpio_directory_open(m, mem);
  if (p) {
    while (*p != 0x00)
      p += 0x20;
  } else {
    if (mem == MPIO_EXTERNAL_MEM) 
      p = m->external.dir;
    if (mem == MPIO_INTERNAL_MEM) 
      p = m->internal.dir;
  }

  /* generate vfat filename in UNICODE */
  ic = iconv_open("UNICODE", "ASCII");
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

  back = unicode + 2;
  
  count = filename_size / 13;
  if (filename_size % 13)
    count++;

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
    /* FIXME: */
    slot->alias_checksum = 0x00;   // checksum for 8.3 alias 

    hexdump((char *)slot, 0x20);
    
    slot++;
    count--;
    index = count;
  }

/*   memcpy(p, m->internal.dir+0x220, 0x20); */

/*   p+=0x20; */
  dentry = (mpio_dir_entry_t *)slot;
  memcpy(dentry->name,"AAAAAAAA",8);
  memcpy(dentry->ext,"MP3",3);
  dentry->attr = 0x20;
  dentry->lcase = 0x00;
  dentry->ctime_ms = 0x00;
  dentry->ctime[0] = 0x37;
  dentry->ctime[1] = 0x7b;
  dentry->cdate[0] = 0xfb;
  dentry->cdate[1] = 0x2c;
  dentry->adate[0] = 0xfd;
  dentry->adate[1] = 0x2c;
  dentry->reserved[0] = 0x00;
  dentry->reserved[1] = 0x00;  
  dentry->time[0] = 0x38;
  dentry->time[1] = 0x7b;
  dentry->date[0] = 0xfb;
  dentry->date[1] = 0x2c;
  dentry->size[0] = fsize & 0xff;
  dentry->size[1] = (fsize / 0x100) & 0xff;
  dentry->size[2] = (fsize / 0x10000) & 0xff;
  dentry->size[3] = (fsize / 0x1000000) & 0xff;
  dentry->start[0] = ssector & 0xff;
  dentry->start[1] = ssector / 0x100;

  free(unicode);
  free(fname);

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
    mpio_dentry_get_real (m, p,
			  fname, 128,
			  fname_8_3,
			  &wdummy, &bdummy, &bdummy,
			  &bdummy, &bdummy, &ddummy);
    if ((strcmp(fname_8_3, filename) == 0) &&
	(strcmp(filename,fname_8_3) == 0)) {
      found = p;
      p = NULL;
    }
    
    p = mpio_dentry_next(m, p);
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
    mpio_dentry_get (m, p,
		     fname, 128,
		     &wdummy, &bdummy, &bdummy,
		     &bdummy, &bdummy, &ddummy);
    if ((strcmp(fname,filename) == 0) && (strcmp(filename,fname) == 0)) {
      found = p;
      p = NULL;
    }
    
    p = mpio_dentry_next(m, p);
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
    debug("could not find file: %s\n", filename);
    return 0;    
  } 

  size = mpio_dentry_get_size(m, start);
  
  if (size <= 0) {
    debug("fatal error in mpio_dentry_delete\n");
    return 0;    
  }

  debugn(5, "size: %2x\n", size);

  /* clear new buffer */
  memset(tmp, 0, DIR_SIZE);
  /* copy before delete */
  if (start != sm->dir)
    memcpy(tmp, sm->dir, (start - (sm->dir)));
  /* copy after delete */
  memcpy(tmp + (start - (sm->dir)), (start + size),
	 (sm->dir + DIR_SIZE - (start + size)));

  memcpy(sm->dir, tmp, DIR_SIZE);
  
  return 0;
}
