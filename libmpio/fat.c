/* 
 *
 * $Id: fat.c,v 1.3 2002/09/03 21:20:53 germeier Exp $
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

#include "fat.h"
#include "io.h"
#include "debug.h"

#include <string.h>
#include <stdlib.h>

int
mpio_bootblocks_read (mpio_t *m, mpio_mem_t mem)
{
  BYTE *pe;  /* partition entry */
  BYTE *bpb; /* BIOS Parameter Block */

  mpio_smartmedia_t *sm=0;  

  int sector, head, cylinder, total_sector;
  long temp;

  /* this should not be needed for internal memory, but ... */
  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;
  if (!sm)
    return 1;

  /* TODO: check a few things more, just to be sure */

  /* read CIS (just in case it might me usefull) */
  /* fixed @ offset 0x20 */
  if (mpio_io_sector_read(m, mem, 0x20, sm->cis)) 
    {
      debug("error reading CIS\n");    
      return 1;
    }

  /* read MBR */
  /* fixed @ offset 0x40 */
  if (mpio_io_sector_read(m, mem, 0x40, sm->mbr)) 
    {
      debug("error reading MBR\n");    
      return 1;
    }

  if ((sm->mbr[0x1fe] != 0x55) || (sm->mbr[0x1ff] != 0xaa)) 
    {
      debug("This is not the MBR!\n");
      return 1;
    }
  
  /* always use the first partition */
  /* we probably don't need to support others */
  pe = sm->mbr + 0x1be;
  
  head = (int)(*(pe+0x01) & 0xff);
  sector = (int)(*(pe+0x02) & 0x3f);
  cylinder = (int)((*(pe+0x02) >> 6) * 0x100 + *(pe + 0x03));
  
  sm->pbr_offset=(cylinder * sm->geo.NumHead + head ) *
    sm->geo.NumSector + sector - 1 + OFFSET_MBR; 

  /* read PBR */
  if (mpio_io_sector_read(m, mem, sm->pbr_offset, sm->pbr))
    {
      debug("error reading PBR\n");    
      return 1;
    }

  if ((sm->pbr[0x1fe] != 0x55) || (sm->pbr[0x1ff] != 0xaa)) 
    {
      debug("This is not the PBR!\n");
      return 1;
    }

  if (strncmp((sm->pbr+0x36),"FAT", 3) != 0) 
    {
      debug("Did not find an FAT signature, *not* good!, aborting!\n");
      exit (1);
    }

  bpb = sm->pbr + 0x0b;
  
  total_sector = (*(sm->pbr+0x14) * 0x100 + *(sm->pbr + 0x13));
  if (!total_sector)
      total_sector = (*(sm->pbr+0x23) * 0x1000000 + 
		      *(sm->pbr+0x22) * 0x10000   +
		      *(sm->pbr+0x21) * 0x100     +
		      *(sm->pbr+0x20));

   /* 128 MB need 2 Bytes instead of 1.5 */
   if (sm->size != 128)
     {
       temp = ((total_sector / 0x20 * 0x03 / 0x02 / 0x200) + 0x01);   
     } else {       
       temp = ((total_sector / 0x20 * 0x02 / 0x200) + 0x01);   
     }   

  sm->max_cluster = (total_sector / BLOCK_SECTORS);
  debugn(2,"max_cluster: %d\n", sm->max_cluster);
  sm->fat_offset = sm->pbr_offset + 0x01;
  sm->fat_size = temp;
  sm->fat_nums = *(sm->pbr + 0x10);
  sm->dir_offset = sm->pbr_offset + 0x01 +  temp * 2;

  return 0;  
}

mpio_fatentry_t *
mpio_fatentry_new(mpio_t *m, mpio_mem_t mem, DWORD sector)
{
  mpio_smartmedia_t *sm;
  mpio_fatentry_t *new;
  
  new = malloc (sizeof(mpio_fatentry_t));
  
  if (new) 
    {
      new->m      = m;
      new->mem    = mem;
      new->entry  = sector;      
    }  
  
    return new;
}
  
int
mpio_fatentry_plus_plus(mpio_fatentry_t *f)
{
  f->entry++;

  if (f->mem == MPIO_INTERNAL_MEM) {
    if (f->entry > f->m->internal.max_cluster)
      return 0;    
  }
  
  if (f->mem == MPIO_EXTERNAL_MEM) {
    if (f->entry > (f->m->external.max_cluster - 2))
      return 0;    
  }
  
  return 1;
}


/* read "fat_size" sectors of fat into the provided buffer */
int
mpio_fat_read (mpio_t *m, mpio_mem_t mem)
{
  mpio_smartmedia_t *sm;
  BYTE recvbuff[SECTOR_SIZE];
  int i;

  if (mem == MPIO_INTERNAL_MEM) 
    {    
      sm = &m->internal;
      if (mpio_io_spare_read(m, mem, 0, (sm->size / sm->chips), 0, sm->fat,
			     (sm->fat_size * SECTOR_SIZE)))
	return 1;
      return 0;
    }
  
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  if (!sm)
    return 1;
  
  for (i = 0; i < sm->fat_size; i++) {
    if (mpio_io_sector_read(m, mem, (sm->fat_offset + i), recvbuff))
      return 1;

    memcpy(sm->fat + (i * SECTOR_SIZE), recvbuff, SECTOR_SIZE);
  }

  return (0);
}

int
mpio_fatentry_free(mpio_t *m, mpio_mem_t mem, mpio_fatentry_t *f )
{
  int e;
  mpio_smartmedia_t *sm;  
  
  if (mem == MPIO_INTERNAL_MEM) {    
    sm = &m->internal;
    e  = f->entry * 0x10;

    if((sm->fat[e+0] == 0xff) &&
       (sm->fat[e+1] == 0xff) &&
       (sm->fat[e+2] == 0xff))
      return 1;
  }

  if (mem == MPIO_EXTERNAL_MEM) {    
    sm = &m->internal;
    if (mpio_fatentry_read(m, mem, f) == 0)
      return 1;
  }

  return 0;
}

DWORD
mpio_fatentry_read(mpio_t *m, mpio_mem_t mem, mpio_fatentry_t *f )
{
  mpio_smartmedia_t *sm;  
  int e;  
  int v;
  
  if (mem == MPIO_INTERNAL_MEM) {    
    sm = &m->internal;
    e  = f->entry;    
    e  = e * 0x10 + 7;
    if((sm->fat[e+0] == 0xff) &&
       (sm->fat[e+1] == 0xff) &&
       (sm->fat[e+2] == 0xff) &&
       (sm->fat[e+3] == 0xff))
      return 0xffffffff;
       
    v = sm->fat[e+0] * 0x1000000 +
        sm->fat[e+1] * 0x10000 +
        sm->fat[e+2] * 0x100 + 
        sm->fat[e+3];    
  
    return v; 
  }
  
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  if (sm->size == 128) {
    /* 2 Byte per entry */
    e = f->entry * 2;
    v = sm->fat[e + 1] * 0x100 + sm->fat[e];
  } else {
    /* 1.5 Byte per entry */
    /* Nibble order: x321 */
    e = (f->entry * 3 / 2);
/*     printf("mager: byte (%d/%d)\n", e, e+1); */
    if ((f->entry & 0x01) == 0) {
      /* LLxH */
      /* 21x3 */
      v = (sm->fat[e + 1] & 0x0f) * 0x100 + sm->fat[e];
    } else {
      /* 1x32 */
      v = (sm->fat[e + 1] * 0x10) + (sm->fat[e] >> 4);
    }    
  }

  return v;  
}

int 
mpio_fatentry_write(mpio_t *m, mpio_mem_t mem, mpio_fatentry_t *f, WORD value)
{
  mpio_smartmedia_t *sm;  
  int e;
  BYTE backup;

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  if (sm->size == 128) 
    {
      /* 2 Byte per entry */
      e = f->entry * 2;
      sm->fat[e]     = value & 0xff;
      sm->fat[e + 1] = (value >> 8 ) & 0xff;      
    } else {
      /* 1.5 Byte per entry */
      e = (f->entry * 3 / 2);
      if ((f->entry & 0x01) == 0) {
	/* 21x3 */
	sm->fat[e]     = value & 0xff;
	backup         = sm->fat[e + 1] & 0xf0;
	sm->fat[e + 1] = backup | (( value / 0x100  ) & 0x0f);
      } else {
	/* 1x32 */
	sm->fat[e + 1] = (value / 0x10) & 0xff;
	backup         = sm->fat[e] & 0x0f;
	sm->fat[e]     = backup | ( (value * 0x10) & 0xf0 );
      }      
    }  

  return 0;
}

int 
mpio_fat_internal_find_startsector(mpio_t *m, BYTE start)
{
  mpio_fatentry_t *f;
  mpio_smartmedia_t *sm = &m->internal;
  int found=-1;

  f = mpio_fatentry_new(m, MPIO_INTERNAL_MEM, 0);

  while(mpio_fatentry_plus_plus(f))
    {
      if ((sm->fat[f->entry * 0x10]     == 0xaa) &&
	  (sm->fat[f->entry * 0x10 + 1] == start)) 
	found=f->entry;
    }

  free(f);

  return found;
}

int  
mpio_fat_free_clusters(mpio_t *m, mpio_mem_t mem)
{
  mpio_smartmedia_t *sm;  
  mpio_fatentry_t *f;
  int i;
  int e = 0;
  int fsize;

  f = mpio_fatentry_new(m, mem, 0);
  
  do 
    {
      if (mpio_fatentry_free(m, mem, f)) e++;      
    } while (mpio_fatentry_plus_plus(f));

  free(f);
    
  return (e * 16);
}

mpio_fatentry_t *
mpio_fat_find_free(mpio_t *m, mpio_mem_t mem)
{
  mpio_fatentry_t *f;

  f = mpio_fatentry_new(m, mem, 1);

  while(mpio_fatentry_plus_plus(f))
    {
      if (mpio_fatentry_free(m, mem, f))
	return f;
    }

  free(f);

  return NULL;
}

int              
mpio_fatentry_next_free(mpio_t *m, mpio_mem_t mem, mpio_fatentry_t *f)
{
  mpio_fatentry_t backup;

  memcpy(&backup, f, sizeof(mpio_fatentry_t));

  while(mpio_fatentry_plus_plus(f))
    {
      if (mpio_fatentry_free(m, mem, f))
	return 1;
    }

  /* no free entry found, restore entry */
  memcpy(f, &backup, sizeof(mpio_fatentry_t));

  return 0;
}

int              
mpio_fatentry_next_entry(mpio_t *m, mpio_mem_t mem, mpio_fatentry_t *f)
{
  mpio_smartmedia_t *sm;
  DWORD value;
  DWORD endvalue;
  BYTE chip;

  value    = mpio_fatentry_read(m, mem, f);
  f->entry = value;
  
  if (mem == MPIO_INTERNAL_MEM) 
    {
      sm = &m->internal;      
      f->hw_address = value;

      if (value == 0xffffffff)
	return 0;

      chip = value >> 24;

      value &= 0xffffff;
      value /= 0x20;
      value += (chip-1) 
	* (m->internal.fat_size * SECTOR_SIZE / 0x10 / m->internal.chips);  


/* this is the opposite code in  mpio_dentry_get_startcluster */
/*       cluster += */
/* 	0x01000000 * ((cluster / 0x20 / (m->internal.fat_size * SECTOR_SIZE / */
/* 				  0x10 / m->internal.chips)) + 1); */

      
      f->entry = value;
      
    }
  

  if (mem == MPIO_EXTERNAL_MEM) 
    {      
      sm    = &m->external;
      
      if (sm->size==128) 
	{
	  endvalue = 0xfff8;
	} else {
	  endvalue = 0xff8;
	} 
      
      if (value >= endvalue)
	return 0;
	  
    }  

  return 1;
}


int
mpio_fat_clear(mpio_t *m, mpio_mem_t mem)
{
  mpio_smartmedia_t *sm;
  
  if (mem == MPIO_INTERNAL_MEM) {    
    sm = &m->internal;
    debug("clearing of the internal FAT not yet supported , sorry\n");
    return 0;
  }
  
  if (mem == MPIO_EXTERNAL_MEM) {
    sm = &m->external;
    memset(sm->fat, 0x00, (sm->fat_size * SECTOR_SIZE));
    sm->fat[0] = 0xf8;
    sm->fat[1] = 0xff;
    sm->fat[2] = 0xff;
    /* for FAT 16 */
    if (sm->size == 128)      
      sm->fat[3] = 0xff;
  }
  
  return 0;
}

/* 
 * This function acutally writes both,
 * the FAT _and_ the root directory
 *
 */

int 
mpio_fat_write(mpio_t *m, mpio_mem_t mem)
{
  mpio_smartmedia_t *sm;
  BYTE dummy[BLOCK_SIZE];
  WORD i;
  
  if (mem == MPIO_INTERNAL_MEM) {    
    sm = &m->internal;
    debug("writing of the internal FAT is not yet supported, sorry\n");
    return 0;
  }
  
  if (mem == MPIO_EXTERNAL_MEM) sm=&m->external;

  memset(dummy, 0xff, BLOCK_SIZE);
  
  for (i = 0x40; i < (sm->dir_offset + DIR_NUM) ; i++) {
    if (((i / 0x20) * 0x20) == i) {
/*       debug("formatting: %2x\n", i); */
      mpio_io_block_delete(m, mem, i, sm->size);
    }
    
    if (i == 0x40)
      mpio_io_sector_write(m, mem, 0x40, sm->mbr);
    if ((i > 0x40) && (i < sm->pbr_offset))
      mpio_io_sector_write(m, mem, i, dummy);
      
    if (i == sm->pbr_offset)
      mpio_io_sector_write(m, mem, sm->pbr_offset, sm->pbr);
    
    if ((i >= sm->fat_offset) && (i < (sm->fat_offset + (2 * sm->fat_size)))) 
      mpio_io_sector_write(m, mem, i, 
			   (sm->fat + SECTOR_SIZE *
			    ((i - sm->fat_offset) % sm->fat_size)));
    
    if (i>=sm->dir_offset)
      mpio_io_sector_write(m, mem, i, 
			   (sm->dir + (i - sm->dir_offset) * SECTOR_SIZE));
  }
    
  return 0;
}