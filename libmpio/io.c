/* -*- linux-c -*- */

/* 
 *
 * $Id: io.c,v 1.25 2003/04/18 13:53:01 germeier Exp $
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

/* 
 *
 * low level I/O 
 *
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"
#include "debug.h"
#include "ecc.h"

BYTE model2externalmem(mpio_model_t);
DWORD blockaddress_encode(DWORD);
DWORD blockaddress_decode(BYTE *);
void fatentry2hw(mpio_fatentry_t *, BYTE *, DWORD *);

/* small hack to handle external addressing on different MPIO models */
BYTE 
model2externalmem(mpio_model_t model)
{
  BYTE m;

  switch(model) 
    {
    case MPIO_MODEL_DMG:
    case MPIO_MODEL_DMG_PLUS:
      m = 0x80;
      break;
    default:
      m = 0x10;
    }

  return m;
}      

/* encoding and decoding of blockaddresses */

DWORD 
blockaddress_encode(DWORD ba)
{
  WORD addr;
  BYTE p = 0, c = 0;
  BYTE high, low;

  high = 0x10 | ((ba / 0x80) & 0x07);
  low  = (ba * 2) & 0xff;
    
  c = high;
  while (c) 
    {
      if (c & 0x01)
	p ^= 1;
      c /= 2;
    }

  c = low;
  while (c) 
    {
      if (c & 0x01)
	p ^= 0x01;
      c /= 2;
    }  
  
  addr = (high * 0x100) + low + p;

  return addr;
}


DWORD 
blockaddress_decode(BYTE *entry)
{
  WORD ba;
  WORD value;
  BYTE p=0, high, low;
  int i, t;

  /* check for "easy" defect block */
  t=1;
  for(i=0; i<0x10; i++)
    if (entry[i] != 0)
      t=0;
  if (t)
    return MPIO_BLOCK_DEFECT;

  /* check for "easy" defect block */
  t=1;
  for(i=0; i<0x10; i++)
    if (entry[i] != 0xff)
      t=0;
  if (t)
    return MPIO_BLOCK_FREE;

  /* check for "strange" errors */
  if ((entry[6] != entry[11]) ||
      (entry[7] != entry[12]))
    {	  
      debug("error: different block addresses found:\n");
      hexdumpn(1, entry, 0x10);
      return MPIO_BLOCK_DEFECT;
    } 
  
  ba = entry[6] * 0x100 + entry[7];
  
  /* special values */
  if (ba == 0xffff)
    return MPIO_BLOCK_DEFECT;
  if (ba == 0x0000)
    return MPIO_BLOCK_CIS;

  /* check parity */
  value = ba;
  while (value)
    {
      if (value & 0x01)
	p ^= 0x01;
      
      value /= 2;
    }
  if (p) 
    {
      debug("parity error found in block address: %2x\n", ba);
      return MPIO_BLOCK_DEFECT;      
    }
  
  high = ((ba / 0x100) & 0x07);
  low  = ((ba & 0x0ff) / 2);
  
  return (high * 0x80 + low);
}

/* foobar  */

void 
fatentry2hw(mpio_fatentry_t *f, BYTE *chip, DWORD *address)
{
  mpio_smartmedia_t *sm;
  
  if (f->mem == MPIO_INTERNAL_MEM) 
    {
      sm       = &f->m->internal;
      /*       hexdump((char *)&f->entry, 4); */
      /*       hexdump((char *)&f->hw_address, 4); */
      *chip    = f->hw_address / 0x1000000;    
      *address = f->hw_address & 0x0ffffff;
    }
  if (f->mem == MPIO_EXTERNAL_MEM) 
    {
      sm        = &f->m->external;
      *chip     = MPIO_EXTERNAL_MEM;
      *address  = mpio_zone_block_find_log(f->m, f->mem, f->entry);
      debugn(3, "mager: %06x (logical: %04x)\n", *address, f->entry);
    }
  return;
}

/*
 * zone management
 */

int   
mpio_zone_init(mpio_t *m, mpio_cmd_t mem)
{
  mpio_smartmedia_t *sm;
  int i;
  int zone, block, e;
  
  if (mem != MPIO_EXTERNAL_MEM) 
    {
      debug("called function with wrong memory selection!\n");
      return -1;
    }
  sm = &m->external;

  for(i=0; i<sm->max_blocks; i++)
    {
      zone = i / MPIO_ZONE_PBLOCKS;
      block= i % MPIO_ZONE_PBLOCKS;

      e = i * 0x10;

      sm->zonetable[zone][block]=blockaddress_decode(sm->spare+e);
      
      hexdumpn(4, sm->spare+e, 0x10);
      debugn(2, "decoded: %04x\n", sm->zonetable[zone][block]);
    }

}

DWORD 
mpio_zone_block_find_log(mpio_t *m, mpio_cmd_t mem, DWORD lblock)
{
  mpio_smartmedia_t *sm;
  int v;

  if (mem != MPIO_EXTERNAL_MEM) 
    {
      debug("called function with wrong memory selection!\n");
      return -1;
    }
  sm = &m->external;
  
  /* OK, I can't explain this right now, but it does work,
   * see Software Driver v3.0, page 31
   */
  v = lblock + (sm->size/64);
  
  return (mpio_zone_block_find_seq(m, mem, v));
}

DWORD 
mpio_zone_block_find_seq(mpio_t *m, mpio_cmd_t mem, DWORD lblock)
{
  mpio_smartmedia_t *sm;
  int i, f, v;
  int zone, block;
  
  if (mem != MPIO_EXTERNAL_MEM) 
    {
      debug("called function with wrong memory selection!\n");
      return -1;
    }
  sm = &m->external;

  if ((lblock>=MPIO_BLOCK_CIS) && (lblock<(MPIO_BLOCK_CIS + BLOCK_SECTORS)))
    {
      zone  = 0;
      block = MPIO_BLOCK_CIS;
    } else {  
      zone  = lblock / MPIO_ZONE_LBLOCKS;
      block = lblock % MPIO_ZONE_LBLOCKS;
    }
  
  f=0;
  for (i=(MPIO_ZONE_PBLOCKS-1); i >=0; i--)
    {

      if (sm->zonetable[zone][i]==block)
	{
	  f++;
	  v=i;
	}
    }

  if (f>1)
    debug("found more than one block, using first one\n");
  
  if (!f) 
    {      
      debugn(2, "block not found\n");
      return MPIO_BLOCK_NOT_FOUND;
    }
  
  return ((zone * BLOCK_SECTORS * MPIO_ZONE_PBLOCKS ) + v * BLOCK_SECTORS);
}

DWORD
mpio_zone_block_set_free(mpio_t *m, mpio_cmd_t mem, DWORD lblock)
{
  DWORD value;
  int zone, block;
  mpio_smartmedia_t *sm;

  if (mem != MPIO_EXTERNAL_MEM)
    {
      debug("called function with wrong memory selection!\n");
      return -1;
    }
  sm = &m->external;

  value = mpio_zone_block_find_log(m, mem, lblock);

  mpio_zone_block_set_free_phys(m, mem, value);
  
  return value;
}

void
mpio_zone_block_set_free_phys(mpio_t *m, mpio_cmd_t mem, DWORD value)
{
  int zone, block;
  mpio_smartmedia_t *sm;

  if (mem != MPIO_EXTERNAL_MEM) 
    {
      debug("called function with wrong memory selection!\n");
      return;
    }
  sm = &m->external;

  zone  = value / BLOCK_SECTORS;
  block = zone  % MPIO_ZONE_PBLOCKS;
  zone  = zone  / MPIO_ZONE_PBLOCKS;

  sm->zonetable[zone][block] = MPIO_BLOCK_FREE;

  return;
}

void
mpio_zone_block_set_defect_phys(mpio_t *m, mpio_cmd_t mem, DWORD value)
{
  int zone, block;
  mpio_smartmedia_t *sm;

  if (mem != MPIO_EXTERNAL_MEM) 
    {
      debug("called function with wrong memory selection!\n");
      return;
    }
  sm = &m->external;

  zone  = value / BLOCK_SECTORS;
  block = zone  % MPIO_ZONE_PBLOCKS;
  zone  = zone  / MPIO_ZONE_PBLOCKS;

  sm->zonetable[zone][block] = MPIO_BLOCK_DEFECT;

  return;
}

void
mpio_zone_block_set(mpio_t *m, mpio_cmd_t mem, DWORD pblock)
{
  int zone, block, pb;
  
  pb    = pblock / BLOCK_SECTORS;
  zone  = pb / MPIO_ZONE_PBLOCKS;
  block = pb % MPIO_ZONE_PBLOCKS;

  m->external.zonetable[zone][block] = MPIO_BLOCK_FREE ;

}

DWORD 
mpio_zone_block_find_free_log(mpio_t *m, mpio_cmd_t mem, DWORD lblock)
{
  mpio_smartmedia_t *sm;
  int v;

  if (mem != MPIO_EXTERNAL_MEM) 
    {
      debug("called function with wrong memory selection!\n");
      return -1;
    }
  sm = &m->external;
  
  /* OK, I can't explain this right now, but it does work,
   * see Software Driver v3.0, page 31
   */
  v = lblock + (sm->size/64);
  
  return (mpio_zone_block_find_free_seq(m, mem, v));
}

DWORD 
mpio_zone_block_find_free_seq(mpio_t *m, mpio_cmd_t mem, DWORD lblock)
{
  DWORD value;
  int zone, block, i;
  mpio_smartmedia_t *sm;

  if (mem != MPIO_EXTERNAL_MEM) 
    {
      debug("called function with wrong memory selection!\n");
      return -1;
    }
  sm = &m->external;
  
  value = mpio_zone_block_find_seq(m, mem, lblock);

  if (value != MPIO_BLOCK_NOT_FOUND)
    {
      debug("logical block numbers is already assigned! (%4x)\n", lblock);
      exit (-1);
    }

  if ((lblock>=MPIO_BLOCK_CIS) && (lblock<(MPIO_BLOCK_CIS + BLOCK_SECTORS)))
    {
      zone  = 0;
      block = MPIO_BLOCK_CIS;
    } else {  
      zone  = lblock / MPIO_ZONE_LBLOCKS;
      block = lblock % MPIO_ZONE_LBLOCKS;
    }
  
  i=0;
  while ((sm->zonetable[zone][i]!=MPIO_BLOCK_FREE) && (i<MPIO_ZONE_PBLOCKS))
    i++;

  if (i==MPIO_ZONE_PBLOCKS)
    {
      debug("could not find free pysical block\n");
      return MPIO_BLOCK_NOT_FOUND;
    }

  debugn(2, "set new sector in zonetable, [%d][%d] = %4x\n", zone, i, block);
  
  sm->zonetable[zone][i] = block;

  return ((zone * BLOCK_SECTORS * MPIO_ZONE_PBLOCKS ) + i * BLOCK_SECTORS);
}


WORD 
mpio_zone_block_get_logical(mpio_t *m, mpio_cmd_t mem, DWORD pblock)
{
  int zone, block, pb;
  
  pb    = pblock / BLOCK_SECTORS;
  zone  = pb / MPIO_ZONE_PBLOCKS;
  block = pb % MPIO_ZONE_PBLOCKS;
  
  return m->external.zonetable[zone][block];
}


  


/*
 * low-low level functions
 */

/* 
 * Set command packet  
 *
 * parameter:
 *
 * cmd:       commando code
 * mem:       internal/external
 * index:     sector/block
 * size:      size of used memory chip
 * wsize:     write size, only for PUT_BLOCK
 * buffer:    output buffer of command packet
 *
 */

int
mpio_io_set_cmdpacket(mpio_t *m, mpio_cmd_t cmd, mpio_mem_t mem, DWORD index,
		      WORD size, BYTE wsize, BYTE *buffer) 
{
  BYTE memory;

  /* clear cmdpacket*/
  memset(buffer, 0, 0x40);  

  *buffer = cmd;
  memory = mem;
  if (mem == MPIO_EXTERNAL_MEM)
    memory = model2externalmem(m->model);
  
  *(buffer + 0x01) =   memory;
  *(buffer + 0x03) =   (BYTE) (index & 0x00ff);
  *(buffer + 0x04) =   (BYTE)((index & 0xff00) >> 8);
  /*  SM cards with less or equal 32 MB only need 2 Bytes
   *  to address sectors or blocks.
   *  The highest byte has to be 0xff in that case!
   */
  if (size <= 32) 
    {
      *(buffer + 0x05) = 0xff;
    } else {  
      *(buffer + 0x05) = (BYTE) (index >> 16);
    }  
  /* is this always 0x48 in case of a block write ?? */
  *(buffer + 0x06) = wsize;  

  memcpy((buffer + 0x3b), "jykim", 5);

  return (0);
}

/*
 * write chunk of data to MPIO filedescriptor
 *
 * parameter:
 *
 * fd:        filedescriptor
 * block:     data buffer (has to be num_bytes)
 * num_bytes: size of data buffer
 *
 */
int
mpio_io_bulk_write(int fd, BYTE *block, int num_bytes)
{
  BYTE *p;
  int count, bytes_left, bytes_written;

  bytes_left = num_bytes;
  bytes_written = 0;
  p = block;

  do
  {
    count = write (fd, p, bytes_left);
    if (count > 0)
    {
      p += count;
      bytes_written += count;
      bytes_left -= count;
    }
  } while (bytes_left > 0 && count > 0);

  return bytes_written;
}

/*
 * read chunk of data from MPIO filedescriptor
 *
 * parameter:
 *
 * fd:        filedescriptor
 * block:     return buffer (has to be num_bytes)
 * num_bytes: size of return buffer
 *
 */
int
mpio_io_bulk_read (int fd, BYTE *block, int num_bytes)
{
  int total_read, count, bytes_left;
  BYTE *p;

  total_read = 0;
  bytes_left = num_bytes;
  p = block;

  do
  {
    count = read (fd, p, bytes_left);

    if (count > 0)
      {
        total_read += count;
	bytes_left -= count;
	p += count;
      }
  } while (total_read < num_bytes && count > 0);

  return total_read;
}

/*
 * low level functions
 */

/* 
 * read version block from MPIO
 *
 * parameter:
 *
 * m:         mpio context
 * buffer:    return buffer (has to be CMD_SIZE)
 *
 */
int
mpio_io_version_read(mpio_t *m, BYTE *buffer)
{
  int nwrite, nread;
  BYTE cmdpacket[CMD_SIZE], status[CMD_SIZE];

  /*  Send command packet to MPIO  */
  mpio_io_set_cmdpacket (m, GET_VERSION, 0, 0, 0xff, 0, cmdpacket);

  debugn  (5, ">>> MPIO\n");
  hexdump (cmdpacket, sizeof(cmdpacket));

  nwrite = mpio_io_bulk_write (m->fd, cmdpacket, 0x40);

  if (nwrite != CMD_SIZE) 
    {
      debug ("Failed to send command.\n\n");
      close (m->fd);
      return 0;    
    }

  /*  Receive packet from MPIO  */
  nread = mpio_io_bulk_read (m->fd, status, 0x40);

  if (nread == -1 || nread != 0x40) 
    {
      debug ("Failed to read Sector.\n%x\n",nread);
      close (m->fd);
      return 0;    
    }

  debugn (5, "<<< MPIO\n");
  hexdump (status, 0x40);

  memcpy(buffer, status, 0x40);
  
  return CMD_SIZE;
}

/* 
 * read sector from SmartMedia
 *
 * parameter:
 *
 * m:         mpio context
 * mem:       MPIO_{INTERNAL,EXTERNAL}_MEM
 * index:     index number of sector to read
 * output:    return buffer (has to be SECTOR_SIZE)
 *
 */

/* right now we assume we only want to read single sectors from 
 * the _first_ internal memory chip 
 */

int
mpio_io_sector_read(mpio_t *m, BYTE mem, DWORD index, BYTE *output)
{
  mpio_smartmedia_t *sm=0;
  DWORD sector;
  int nwrite, nread;
  BYTE cmdpacket[CMD_SIZE], recvbuff[SECTOR_TRANS];

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;
  if (!sm)
    {
      debug("error in memory selection, aborting\n");      
      exit (-1);
    }

  if (mem == MPIO_INTERNAL_MEM)
    {
      sector = index;
    } else {      
      /* find the correct physical block first! */
      if ((index>=MPIO_BLOCK_CIS) && (index<(MPIO_BLOCK_CIS + BLOCK_SECTORS)))
	{
	  sector = mpio_zone_block_find_seq(m, mem, MPIO_BLOCK_CIS);
	  sector+= (index % MPIO_BLOCK_CIS);
	} else {
	  sector = mpio_zone_block_find_seq(m, mem, (index / 0x20));
	  sector+= (index % 0x20);
	}
    }

  debugn (2, "sector: %8x (%06x)\n", index, sector);

  mpio_io_set_cmdpacket (m, GET_SECTOR, mem, sector, sm->size, 0, cmdpacket);

  debugn (5, "\n>>> MPIO\n");
  hexdump (cmdpacket, sizeof(cmdpacket));
    
  nwrite = mpio_io_bulk_write (m->fd, cmdpacket, 0x40);

  if(nwrite != CMD_SIZE) 
    {
      debug ("\nFailed to send command.\n\n");
      close (m->fd);
      return 1;
    }

  /*  Receive packet from MPIO   */
  nread = mpio_io_bulk_read (m->fd, recvbuff, SECTOR_TRANS);

  if(nread != SECTOR_TRANS) 
    {
      debug ("\nFailed to read Sector.\n%x\n", nread);
      close (m->fd);
      return 1;
    }

  /* check ECC Area information */
  if (mem==MPIO_EXTERNAL_MEM) 
    {    
      if (mpio_ecc_256_check (recvbuff,
			      (recvbuff + SECTOR_SIZE + 13)) ||
          mpio_ecc_256_check ((recvbuff + (SECTOR_SIZE / 2)),
			      (recvbuff + SECTOR_SIZE + 8))    )
      	debug ("ECC error @ (%02x : %06x)\n", mem, index);
    }

  /* This should not be needed:
     * we don't have ECC information for the internal memory
     * we only read the directory through this function
   */
  /*   if (mem==MPIO_INTERNAL_MEM)  */
  /*     { */
  /*       debugn(2, "WARNING, code for internal FAT entry (in ECC area)" */
  /* 	    " not yet in place!!\n"); */
  /*     } */
  
  debugn (5, "\n<<< MPIO\n");
  hexdump (recvbuff, SECTOR_TRANS);

  memcpy(output, recvbuff, SECTOR_SIZE);  

  return 0;  
}

/* 
 * write sector to SmartMedia
 *
 * parameter:
 *
 * m:         mpio context
 * mem:       MPIO_{INTERNAL,EXTERNAL}_MEM
 * index:     index number of sector to read
 * input:     data buffer (has to be SECTOR_SIZE)
 *
 */

/* right now we assume we only want to write single sectors from 
 * the _first_ internal memory chip 
 */

int  
mpio_io_sector_write(mpio_t *m, BYTE mem, DWORD index, BYTE *input)
{
  int nwrite;
  mpio_smartmedia_t *sm;
  DWORD pvalue;
  DWORD block_address, ba;
  BYTE cmdpacket[CMD_SIZE], sendbuff[SECTOR_TRANS];

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;
  if (!sm)
    {
      debug("error in memory selection, aborting\n");      
      exit (-1);
    }

  /* we have to:
   * - find the physical block (or allocate a new one)
   * - calculate the logical block for zone management
   */

  if (mem == MPIO_EXTERNAL_MEM) 
    {      
      if (index==MPIO_BLOCK_DEFECT) 
	{
	  block_address = 0;
	  pvalue = 0;
	} else {
	  if ((index>=MPIO_BLOCK_CIS) && 
	      (index<(MPIO_BLOCK_CIS + BLOCK_SECTORS)))
	    {
	      block_address = 0;
	      if (index==MPIO_BLOCK_CIS)
		{
		  pvalue=mpio_zone_block_find_free_seq(m, mem, index);  
		} else {
		  /* find the block from the block list! */
		  pvalue=mpio_zone_block_find_seq(m, mem, MPIO_BLOCK_CIS);
		}
	      if (pvalue != MPIO_BLOCK_NOT_FOUND)
		pvalue = pvalue + index - MPIO_BLOCK_CIS;
	      
	    } else {      
	      block_address = blockaddress_encode(index / BLOCK_SECTORS); 
	      if ((index % BLOCK_SECTORS) == 0)
		{
		  /* this is the first write to the block: allocate a new one */
		  /* ... and mark it with the block_address */
		  pvalue=mpio_zone_block_find_free_seq(m, mem, 
						       (index / BLOCK_SECTORS));      
		} else {
		  /* find the block from the block list! */
		  pvalue=mpio_zone_block_find_seq(m, mem, (index / BLOCK_SECTORS));
		}
	      if (pvalue != MPIO_BLOCK_NOT_FOUND)
		pvalue = pvalue + (index % BLOCK_SECTORS);
	      
	    }
	  
	  if (pvalue == MPIO_BLOCK_NOT_FOUND)
	    {
	      debug ("Oops, this should never happen! (%6x : %6x)\n", 
		     index, block_address);
	      exit (-1);
	    }      
	}
    } else {
      pvalue = index;
    }

  mpio_io_set_cmdpacket(m, PUT_SECTOR, mem, pvalue, sm->size, 0, cmdpacket);

  debugn (5, "\n>>> MPIO\n");
  hexdump (cmdpacket, sizeof(cmdpacket));
    
  nwrite = mpio_io_bulk_write(m->fd, cmdpacket, 0x40);

  if(nwrite != CMD_SIZE) 
    {
      debug ("\nFailed to send command.\n\n");
      close (m->fd);
      return 1;
    }

  memset(sendbuff, 0, SECTOR_TRANS);
  memset(sendbuff + SECTOR_SIZE, 0xff, 0x10);
  memcpy(sendbuff, input, SECTOR_SIZE);
  
  if (mem==MPIO_EXTERNAL_MEM)    
    {    
      if (index == MPIO_BLOCK_DEFECT) {
	memset(sendbuff + SECTOR_SIZE, 0, 0x10);
	mpio_zone_block_set_defect_phys(m, mem, pvalue);
      } else {
	
	/* generate ECC information for spare area ! */
	mpio_ecc_256_gen(sendbuff, 
			 sendbuff + SECTOR_SIZE + 0x0d);
	mpio_ecc_256_gen(sendbuff + (SECTOR_SIZE / 2), 
			 sendbuff + SECTOR_SIZE + 0x08);
	if (index == MPIO_BLOCK_DEFECT) {
	  memset(sendbuff + SECTOR_SIZE, 0, 0x10);
	} else {	  
	  if (index == MPIO_BLOCK_CIS) {
	    memset(sendbuff + SECTOR_SIZE + 0x06, 0, 2);
	    memset(sendbuff + SECTOR_SIZE + 0x0b, 0, 2);
	  } else {	  
	    ba = (block_address / 0x100) & 0xff;
	    sendbuff[SECTOR_SIZE + 0x06] = ba;
	    sendbuff[SECTOR_SIZE + 0x0b] = ba;
	    
	    ba = block_address & 0xff;
	    sendbuff[SECTOR_SIZE + 0x07] = ba;
	    sendbuff[SECTOR_SIZE + 0x0c] = ba;
	  }
	}
	
      }
    }
  
  /* easy but working, we write back the FAT info we read before */
  if (mem==MPIO_INTERNAL_MEM) 
      memcpy((sendbuff+SECTOR_SIZE), sm->fat, 0x10);

  debugn (5, "\n>>> MPIO\n");
  hexdump(sendbuff, SECTOR_TRANS);

  /*  write sector MPIO   */
  nwrite = mpio_io_bulk_write(m->fd, sendbuff, SECTOR_TRANS);

  if(nwrite != SECTOR_TRANS) 
    {
      debug ("\nFailed to read Sector.\n%x\n", nwrite);
      close (m->fd);
      return 1;
    }

  return 0;  
}

/*
 * read/write of blocks
 */
int
mpio_io_block_read(mpio_t *m, mpio_mem_t mem, mpio_fatentry_t *f, BYTE *output)
{
  int i=0;
  int nwrite, nread;
  mpio_smartmedia_t *sm;
  BYTE  chip;
  DWORD address;
  BYTE cmdpacket[CMD_SIZE], recvbuff[BLOCK_TRANS];

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  fatentry2hw(f, &chip, &address);

  mpio_io_set_cmdpacket(m, GET_BLOCK, chip, address, sm->size, 0, cmdpacket);

  debugn(5, "\n>>> MPIO\n");
  hexdump(cmdpacket, sizeof(cmdpacket));
    
  nwrite = mpio_io_bulk_write(m->fd, cmdpacket, CMD_SIZE);

  if(nwrite != CMD_SIZE) 
    {
      debug ("\nFailed to send command.\n\n");
      close (m->fd);
      return 1;
    }

  /*  Receive packet from MPIO   */
  nread = mpio_io_bulk_read(m->fd, recvbuff, BLOCK_TRANS);

  if(nread != BLOCK_TRANS) 
    {
      debug ("\nFailed to read Block.\n%x\n",nread);
      close (m->fd);
      return 1;
    }

  debugn(5, "\n<<< MPIO\n");
  hexdump(recvbuff, BLOCK_TRANS);

  for (i = 0; i < BLOCK_SECTORS; i++) 
    {
      /* check ECC Area information */
      if (mem==MPIO_EXTERNAL_MEM) {      
	if (mpio_ecc_256_check ((recvbuff + (i * SECTOR_TRANS)),
				((recvbuff +(i * SECTOR_TRANS) 
				  + SECTOR_SIZE +13))) ||	
	    mpio_ecc_256_check ((recvbuff + (i * SECTOR_TRANS) 
				 + (SECTOR_SIZE / 2)),
				((recvbuff +(i * SECTOR_TRANS) 
				  + SECTOR_SIZE + 8))))
	  debug ("ECC error @ (%02x : %06x)\n", chip, address);
      }
      
      memcpy(output + (i * SECTOR_SIZE), 
	     recvbuff + (i * SECTOR_TRANS), 
	     SECTOR_SIZE);
    }

  return 0;  
}

/* Read spare is only usefull for the internal memory,
 * the FAT lies there. It is updated during the
 * mpio_io_{sector,block}_{read,write} operations.
 *
 * For external SmartMedia cards we have a "seperate" FAT
 * which is read and updated just like on a regular floppy
 * disc or hard drive.
 *
 */

int
mpio_io_spare_read(mpio_t *m, BYTE mem, DWORD index, WORD size,
		   BYTE wsize, BYTE *output, int toread,
		   mpio_callback_init_t progress_callback)
{
  mpio_smartmedia_t *sm;
  int i;
  int nwrite, nread;
  int chip = 0;
  int chips = 0;
  BYTE cmdpacket[CMD_SIZE];

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;  
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  chips = sm->chips;  
  
  for (chip = 1; chip <= chips; chip++) 
    {
      if (mem == MPIO_INTERNAL_MEM) 
	mpio_io_set_cmdpacket(m, GET_SPARE_AREA, (1 << (chip-1)), 
			      index, (size / sm->chips), 
			      wsize, cmdpacket);
      if (mem == MPIO_EXTERNAL_MEM) 
	mpio_io_set_cmdpacket(m, GET_SPARE_AREA, mem, index, size, 
			      wsize, cmdpacket);
      debugn(5, "\n>>> MPIO\n");
      hexdump(cmdpacket, sizeof(cmdpacket));
      
      nwrite = mpio_io_bulk_write(m->fd, cmdpacket, CMD_SIZE);
      
      if(nwrite != CMD_SIZE) {
	debug ("\nFailed to send command.\n\n");
	close (m->fd);
	return 1;
      }
      
      /*  Receive packet from MPIO   */
      for (i = 0; i < (toread / chips / CMD_SIZE); i++) 
	{	  
	  nread = mpio_io_bulk_read (m->fd,
				     output + (i * CMD_SIZE) +
				     (toread / chips * (chip - 1)),
				     CMD_SIZE);

	  if ((progress_callback) && (i % 256))
	    (*progress_callback)(mem, 
				 (i*CMD_SIZE+(toread/chips*(chip-1))),
				 toread );
      
	  if(nread != CMD_SIZE) 
	    {
	      debug ("\nFailed to read Block.\n%x\n",nread);
	      close (m->fd);
	      return 1;
	    }
	  debugn(5, "\n<<< MPIO\n");
	  hexdump(output + (i * CMD_SIZE) + (toread / chips * (chip - 1)), 
		  CMD_SIZE);
	}
    }
  if (progress_callback)
    (*progress_callback)(mem, toread, toread);
  
  return 0;  
}

int  
mpio_io_block_delete(mpio_t *m, mpio_mem_t mem, mpio_fatentry_t *f)
{
  BYTE  chip=0;
  DWORD address;
  mpio_smartmedia_t *sm;

  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;

  fatentry2hw(f, &chip, &address);

  if (address == MPIO_BLOCK_NOT_FOUND)
    {
      debug("hmm, what happens here? (%4x)\n", f->entry);
      return 0;
    }

  return (mpio_io_block_delete_phys(m, chip, address));
}

int
mpio_io_block_delete_phys(mpio_t *m, BYTE chip, DWORD address)
{
  mpio_smartmedia_t *sm;
  int nwrite, nread;
  BYTE cmdpacket[CMD_SIZE], status[CMD_SIZE];
  
  /*  Send command packet to MPIO  */
  
  if (chip == MPIO_INTERNAL_MEM) sm = &m->internal;
  /* hack to support 2+4 internal chips
   * who ever allowed me to write code??? -mager
   */
  if (chip == (MPIO_INTERNAL_MEM+1)) sm = &m->internal;
  if (chip == (MPIO_INTERNAL_MEM+3)) sm = &m->internal;
  if (chip == (MPIO_INTERNAL_MEM+7)) sm = &m->internal;
  if (chip == MPIO_EXTERNAL_MEM) 
    {
      sm = &m->external;
      mpio_zone_block_set_free_phys(m, chip, address);
    }

  mpio_io_set_cmdpacket(m, DEL_BLOCK, chip, address, sm->size, 0, cmdpacket);

  debugn  (5, ">>> MPIO\n");
  hexdump (cmdpacket, sizeof(cmdpacket));

  nwrite = mpio_io_bulk_write(m->fd, cmdpacket, 0x40);

  if (nwrite != CMD_SIZE) 
    {
      debug ("Failed to send command.\n\n");
      close (m->fd);
      return 0;
    }

/*  Receive packet from MPIO  */
  nread = mpio_io_bulk_read (m->fd, status, CMD_SIZE);

  if ((nread == -1) || (nread != CMD_SIZE)) 
    {
      debug ("Failed to read Sector.\n%x\n",nread);
      close (m->fd);
      return 0;
    }

  debugn(5, "<<< MPIO\n");
  hexdump(status, CMD_SIZE);

  if (status[0] != 0xc0) 
    {
      debugn (2,"error formatting Block %02x:%06x\n", 
	     chip, address);
      if (chip == MPIO_EXTERNAL_MEM) 
	{
	  sm = &m->external;
	  mpio_zone_block_set_defect_phys(m, chip, address);
	}      
    }
  
  return CMD_SIZE;
}

int  
mpio_io_block_write(mpio_t *m, mpio_mem_t mem, mpio_fatentry_t *f, BYTE *data)
{
  mpio_smartmedia_t *sm;
  int nwrite;
  int i;
  DWORD block_address, ba;
  BYTE cmdpacket[CMD_SIZE], sendbuff[BLOCK_TRANS];
  BYTE  chip=0;
  DWORD address;

  if (mem == MPIO_INTERNAL_MEM) 
    {
      sm = &m->internal;
      fatentry2hw(f, &chip, &address);
    }
	
  if (mem == MPIO_EXTERNAL_MEM) 
    {
      sm = &m->external;
      /* find free physical block */
      chip = MPIO_EXTERNAL_MEM;
      address = mpio_zone_block_find_free_log(m, mem, f->entry);
    }

  /* build block for transfer to MPIO */
  for (i = 0; i < BLOCK_SECTORS; i++) 
    {
      memcpy(sendbuff + (i * SECTOR_TRANS), 
	     data + (i * SECTOR_SIZE),	   
	     SECTOR_SIZE);
      memset(sendbuff + (i * SECTOR_TRANS) + SECTOR_SIZE,
	     0xff, CMD_SIZE);

      if (mem == MPIO_INTERNAL_MEM) 
	{	  
	  if (i == 0)
	    {	      
	      memcpy((sendbuff+SECTOR_SIZE+(i * SECTOR_TRANS)), 
		     f->i_fat, 0x10);
/* 	      debug("address %02x:%06x\n", chip, address); */
/* 	      hexdumpn(0, f->i_fat, 0x10); */
	    }	  
	}      

      /* fill in block information */
      if (mem == MPIO_EXTERNAL_MEM) 
	{      
	  block_address = mpio_zone_block_get_logical(m, mem, address);
	  block_address = blockaddress_encode(block_address);

	  ba = (block_address / 0x100) & 0xff;
	  sendbuff[(i * SECTOR_TRANS) + SECTOR_SIZE + 0x06] = ba;
	  sendbuff[(i * SECTOR_TRANS) + SECTOR_SIZE + 0x0b] = ba;
	  
	  ba = block_address & 0xff;
	  sendbuff[(i * SECTOR_TRANS) + SECTOR_SIZE + 0x07] = ba;
	  sendbuff[(i * SECTOR_TRANS) + SECTOR_SIZE + 0x0c] = ba;
	  
	  /* generate ECC Area information */
	  mpio_ecc_256_gen ((sendbuff + (i * SECTOR_TRANS)),                   
			    ((sendbuff + (i * SECTOR_TRANS) 
			      + SECTOR_SIZE + 13)));
	  mpio_ecc_256_gen ((sendbuff + (i * SECTOR_TRANS) 
			      + (SECTOR_SIZE / 2)), 
			    ((sendbuff + (i * SECTOR_TRANS) 
			      + SECTOR_SIZE + 8)));
	}
  }

  mpio_io_set_cmdpacket(m, PUT_BLOCK, chip, address, sm->size, 0x48 , cmdpacket);

  debugn(5, "\n>>> MPIO\n");
  hexdump(cmdpacket, sizeof(cmdpacket));
    
  nwrite = mpio_io_bulk_write(m->fd, cmdpacket, CMD_SIZE);

  if(nwrite != CMD_SIZE) 
    {
      debug ("\nFailed to send command.\n\n");
      close (m->fd);
      return 1;
    }

  /*  send packet to MPIO   */
  debugn(5, "\n<<< MPIO\n");
  hexdump(sendbuff, BLOCK_TRANS);
  nwrite = mpio_io_bulk_write (m->fd, sendbuff, BLOCK_TRANS);

  if(nwrite != BLOCK_TRANS) 
    {
      debug ("\nFailed to read Block.\n%x\n",nwrite);
      close (m->fd);
      return 1;
    }

  return 0;
}
