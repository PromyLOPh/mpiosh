/* -*- linux-c -*- */

/* 
 *
 * $Id: io.c,v 1.1 2002/08/28 16:10:50 salmoon Exp $
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

WORD index2blockaddress(WORD);

WORD index2blockaddress(WORD ba)
{
  WORD addr;
  BYTE p = 0, c = 0;
  BYTE high, low;

  high = 0x10 | ((ba / 0x80) & 0x07);
  low  = (ba * 2) & 0xff;
    
  c = high;
  while (c) {
    if (c & 0x01)
      p ^= 1;
    c /= 2;
  }

  c = low;
  while (c) {
    if (c & 0x01)
      p ^= 0x01;
    c /= 2;
  }  
  
  addr = (high * 0x100) + low + p;

  return addr;
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
 * small:     FAT with less or equal 32MB
 * wsize:     write size, only for PUT_BLOCK
 *
 *
 */

int
mpio_io_set_cmdpacket(mpio_cmd_t cmd, mpio_mem_t mem, DWORD index,
		      BYTE small, BYTE wsize, BYTE *buffer) 
{

  /* clear cmdpacket*/
  memset(buffer, 0, 0x40);  

  *buffer = cmd;
  *(buffer + 0x01) =   mem;
  *(buffer + 0x03) =   (BYTE) (index & 0x00ff);
  *(buffer + 0x04) =   (BYTE)((index & 0xff00) >> 8);
  /*  SM cards with less or equal 32 MB only need 2 Bytes
   *  to address sectors or blocks.
   *  The highest byte has to be 0xff in that case!
   */
  if (small <= 32) {
    *(buffer + 0x05) = 0xff;
  } else {  
    *(buffer + 0x05) = (BYTE) (index >> 16);
  }  
  *(buffer + 0x06) = wsize;  /* is this always 0x48 in case of a write ?? */

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
  mpio_io_set_cmdpacket (GET_VERSION, 0, 0, 0xff, 0, cmdpacket);

  debugn  (5, ">>> MPIO\n");
  hexdump (cmdpacket, sizeof(cmdpacket));

  nwrite = mpio_io_bulk_write (m->fd, cmdpacket, 0x40);

  if (nwrite != CMD_SIZE) {
    debug ("Failed to send command.\n\n");
    close (m->fd);
    return 0;    
  }

  /*  Receive packet from MPIO  */
  nread = mpio_io_bulk_read (m->fd, status, 0x40);

  if (nread == -1 || nread != 0x40) {
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
 * area:      MPIO_{INTERNAL,EXTERNAL}_MEM
 * index:     index number of sector to read
 * small:     size of memory chip to read from
 * wsize:     dummy
 * output:    return buffer (has to be SECTOR_SIZE)
 *
 */
int
mpio_io_sector_read(mpio_t *m, BYTE area, DWORD index, BYTE small,
		    BYTE wsize, BYTE *output)
{
  int nwrite, nread;
  BYTE cmdpacket[CMD_SIZE], recvbuff[SECTOR_TRANS];

  mpio_io_set_cmdpacket (GET_SECTOR, area, index, small, wsize, cmdpacket);

  debugn (5, "\n>>> MPIO\n");
  hexdump (cmdpacket, sizeof(cmdpacket));
    
  nwrite = mpio_io_bulk_write (m->fd, cmdpacket, 0x40);

  if(nwrite != CMD_SIZE) {
    debug ("\nFailed to send command.\n\n");
    close (m->fd);
    return 1;
  }

  /*  Receive packet from MPIO   */
  nread = mpio_io_bulk_read (m->fd, recvbuff, SECTOR_TRANS);

  if(nread != SECTOR_TRANS) {
    debug ("\nFailed to read Sector.\n%x\n", nread);
    close (m->fd);
    return 1;
  }

  /* check ECC Area information */
  if (area==MPIO_EXTERNAL_MEM) {    
    mpio_ecc_256_check (recvbuff,
		  (recvbuff + SECTOR_SIZE + 13));
    mpio_ecc_256_check ((recvbuff + (SECTOR_SIZE / 2)),
		  (recvbuff + SECTOR_SIZE + 8));
  }
  
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
 * area:      MPIO_{INTERNAL,EXTERNAL}_MEM
 * index:     index number of sector to read
 * small:     size of memory chip to read from
 * wsize:     dummy
 * input:     data buffer (has to be SECTOR_SIZE)
 *
 */
int  
mpio_io_sector_write(mpio_t *m, BYTE area, DWORD index, BYTE small,
		     BYTE wsize, BYTE *input)
{
  DWORD block_address, ba;
  int nwrite;
  BYTE cmdpacket[CMD_SIZE], sendbuff[SECTOR_TRANS];

  mpio_io_set_cmdpacket(PUT_SECTOR, area, index, small, wsize, cmdpacket);

  debugn (5, "\n>>> MPIO\n");
  hexdump (cmdpacket, sizeof(cmdpacket));
    
  nwrite = mpio_io_bulk_write(m->fd, cmdpacket, 0x40);

  if(nwrite != CMD_SIZE) {
    debug ("\nFailed to send command.\n\n");
    close (m->fd);
    return 1;
  }

  memset(sendbuff, 0, SECTOR_TRANS);
  memset(sendbuff + SECTOR_SIZE, 0xff, 0x10);
  memcpy(sendbuff, input, SECTOR_SIZE);
  
  if (index<0x40) {
    block_address=0;
  } else {  
    ba= (index /0x20) - 2;
    debugn(2, "foobar: %4x\n", ba);
    block_address= index2blockaddress(ba);
    debugn(2, "foobar: %4x\n", block_address);
  }
  
  /* generate ECC information for spare area ! */
  mpio_ecc_256_gen(sendbuff, 
	     sendbuff + SECTOR_SIZE + 0x0d);
  mpio_ecc_256_gen(sendbuff + (SECTOR_SIZE / 2), 
	     sendbuff + SECTOR_SIZE + 0x08);
  
  ba = (block_address / 0x100) & 0xff;
  sendbuff[SECTOR_SIZE + 0x06] = ba;
  sendbuff[SECTOR_SIZE + 0x0b] = ba;
  
  ba = block_address & 0xff;
  sendbuff[SECTOR_SIZE + 0x07] = ba;
  sendbuff[SECTOR_SIZE + 0x0c] = ba;

  debugn (5, "\n>>> MPIO\n");
  hexdump(sendbuff, SECTOR_TRANS);

  /*  write sector MPIO   */
  nwrite = mpio_io_bulk_write(m->fd, sendbuff, SECTOR_TRANS);

  if(nwrite != SECTOR_TRANS) {
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
mpio_io_block_read(mpio_t *m, BYTE area, DWORD index, BYTE small,
		   BYTE wsize, BYTE *output)
{
  int i=0;
  int nwrite, nread;
  DWORD rarea=0;
  BYTE cmdpacket[CMD_SIZE], recvbuff[BLOCK_TRANS];

  if (area == MPIO_INTERNAL_MEM) {
    rarea = index / 0x1000000;    
    index &= 0xffffff;
  }
  if (area == MPIO_EXTERNAL_MEM) {
    rarea = area;
  }

  index *= BLOCK_SECTORS;

  mpio_io_set_cmdpacket(GET_BLOCK, rarea, index, small, wsize, cmdpacket);

  debugn(5, "\n>>> MPIO\n");
  hexdump(cmdpacket, sizeof(cmdpacket));
    
  nwrite = mpio_io_bulk_write(m->fd, cmdpacket, CMD_SIZE);

  if(nwrite != CMD_SIZE) {
    debug ("\nFailed to send command.\n\n");
    close (m->fd);
    return 1;
  }

  /*  Receive packet from MPIO   */
  nread = mpio_io_bulk_read(m->fd, recvbuff, BLOCK_TRANS);

  if(nread != BLOCK_TRANS) {
    debug ("\nFailed to read Block.\n%x\n",nread);
    close (m->fd);
    return 1;
  }

  debugn(5, "\n<<< MPIO\n");
  hexdump(recvbuff, BLOCK_TRANS);

  for (i = 0; i < BLOCK_SECTORS; i++) {
    /* check ECC Area information */
    if (area==MPIO_EXTERNAL_MEM) {      
      mpio_ecc_256_check ((recvbuff + (i * SECTOR_TRANS)),                   
		    ((recvbuff + (i * SECTOR_TRANS) + SECTOR_SIZE + 13)));
      mpio_ecc_256_check ((recvbuff + (i * SECTOR_TRANS) + (SECTOR_SIZE / 2)), 
		    ((recvbuff + (i * SECTOR_TRANS) + SECTOR_SIZE + 8)));
    }
    
    memcpy(output + (i * SECTOR_SIZE), 
	   recvbuff + (i * SECTOR_TRANS), 
	   SECTOR_SIZE);
  }

  return 0;  
}

/* Read spare is only usefull for the internal memory,
 * the FAT lies there.
 * 
 * For external SmartMedia cards we get and write the
 * informatione during {read,write}_{sector,block}
 *
 */

int
mpio_io_spare_read(mpio_t *m, BYTE area, DWORD index, BYTE small,
		   BYTE wsize, BYTE *output, int toread)
{
  int i;
  int nwrite, nread;
  int chip = 0;
  int chips = 0;
  BYTE cmdpacket[CMD_SIZE];

  if (area != MPIO_INTERNAL_MEM) {
    debug("Something fishy happened, aborting!n");
    exit(1);
  }

  chips = m->internal.chips;  
  
  for (chip = 1; chip <= chips; chip++) {    
    mpio_io_set_cmdpacket(GET_SPARE_AREA, chip, index, small, 
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
    for (i = 0; i < (toread / chips / CMD_SIZE); i++) {
      
      nread = mpio_io_bulk_read (m->fd,
				 output + (i * CMD_SIZE) +
				 (toread / chips * (chip - 1)),
				 CMD_SIZE);
      
      if(nread != CMD_SIZE) {
	debug ("\nFailed to read Block.\n%x\n",nread);
	close (m->fd);
	return 1;
      }
      debugn(5, "\n<<< MPIO\n");
      hexdump(output + (i * CMD_SIZE) + (toread / chips * (chip - 1)), 
	      CMD_SIZE);
    }
  }

  return 0;  
}

int  
mpio_io_block_delete(mpio_t *m, BYTE mem, DWORD index, BYTE small)
{
  int nwrite, nread;
  BYTE cmdpacket[CMD_SIZE], status[CMD_SIZE];

/*  Send command packet to MPIO  */

  mpio_io_set_cmdpacket(DEL_BLOCK, mem, index, small, 0, cmdpacket);

  debugn  (5, ">>> MPIO\n");
  hexdump (cmdpacket, sizeof(cmdpacket));

  nwrite = mpio_io_bulk_write(m->fd, cmdpacket, 0x40);

  if (nwrite != CMD_SIZE) {
    debug ("Failed to send command.\n\n");
    close (m->fd);
    return 0;
  }

/*  Receive packet from MPIO  */
  nread = mpio_io_bulk_read (m->fd, status, CMD_SIZE);

  if ((nread == -1) || (nread != CMD_SIZE)) {
      debug ("Failed to read Sector.\n%x\n",nread);
    close (m->fd);
    return 0;
  }

  if (status[0] != 0xc0) debug ("error formatting Block %04x\n", index);
  debugn(5, "<<< MPIO\n");
  hexdump(status, CMD_SIZE);
  
  return CMD_SIZE;
}

int  
mpio_io_block_write(mpio_t *m, BYTE area, DWORD index, BYTE small, 
		    BYTE wsize, BYTE *data)
{
  int i = 0;
  int nwrite;
  DWORD block_address, ba;
  DWORD rarea = 0;
  BYTE cmdpacket[CMD_SIZE], sendbuff[BLOCK_TRANS];

  if (area == MPIO_INTERNAL_MEM) {
    rarea = index / 0x1000000;    
    index &= 0xffffff;
  }
  if (area == MPIO_EXTERNAL_MEM) {
    rarea = area;
  }

  index *= BLOCK_SECTORS;  

  /* build block for transfer to MPIO */
  for (i = 0; i < BLOCK_SECTORS; i++) {
    memcpy(sendbuff + (i * SECTOR_TRANS), 
	   data + (i * SECTOR_SIZE),	   
	   SECTOR_SIZE);
    memset(sendbuff + (i * SECTOR_TRANS) + SECTOR_SIZE,
	   0xff, CMD_SIZE);
    /* fill in block information */
    if (index < 0x40) {
      block_address = 0;
    } else {  
      ba = (index / 0x20) - 2;
/*       debugn(2, "foobar: %4x\n", ba); */
      block_address = index2blockaddress(ba);
/*       debugn(2, "foobar: %4x\n", block_address); */
    }

    ba = (block_address / 0x100) & 0xff;
    sendbuff[(i * SECTOR_TRANS) + SECTOR_SIZE + 0x06] = ba;
    sendbuff[(i * SECTOR_TRANS) + SECTOR_SIZE + 0x0b] = ba;
    
    ba = block_address & 0xff;
    sendbuff[(i * SECTOR_TRANS) + SECTOR_SIZE + 0x07] = ba;
    sendbuff[(i * SECTOR_TRANS) + SECTOR_SIZE + 0x0c] = ba;

    /* generate ECC Area information */
    if (area == MPIO_EXTERNAL_MEM) {      
      mpio_ecc_256_gen ((sendbuff + (i * SECTOR_TRANS)),                   
			((sendbuff + (i * SECTOR_TRANS) + SECTOR_SIZE + 13)));
      mpio_ecc_256_gen ((sendbuff + (i * SECTOR_TRANS) + (SECTOR_SIZE / 2)), 
			((sendbuff + (i * SECTOR_TRANS) + SECTOR_SIZE + 8)));
    }
  }

  mpio_io_set_cmdpacket(PUT_BLOCK, rarea, index, small, wsize, cmdpacket);

  debugn(5, "\n>>> MPIO\n");
  hexdump(cmdpacket, sizeof(cmdpacket));
    
  nwrite = mpio_io_bulk_write(m->fd, cmdpacket, CMD_SIZE);

  if(nwrite != CMD_SIZE) {
    debug ("\nFailed to send command.\n\n");
    close (m->fd);
    return 1;
  }

  /*  send packet to MPIO   */
  debugn(5, "\n<<< MPIO\n");
  hexdump(sendbuff, BLOCK_TRANS);
  nwrite = mpio_io_bulk_write (m->fd, sendbuff, BLOCK_TRANS);

  if(nwrite != BLOCK_TRANS) {
    debug ("\nFailed to read Block.\n%x\n",nwrite);
    close (m->fd);
    return 1;
  }

  return 0;
}
