/* -*- linux-c -*- */

/* 
 *
 * $Id: smartmedia.c,v 1.2 2002/09/03 10:22:24 germeier Exp $
 *
 * Library for USB MPIO-*
 *
 * Markus Germeier (mager@tzi.de)
 *
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

#include <stdio.h>
#include <stdlib.h>

#include "smartmedia.h"
#include "debug.h"

mpio_disk_phy_t MPIO_DISK_GEO_002={ 125, 4,  8,   4000 };
mpio_disk_phy_t MPIO_DISK_GEO_004={ 250, 4,  8,   8000 };
mpio_disk_phy_t MPIO_DISK_GEO_008={ 250, 4, 16,  16000 };
mpio_disk_phy_t MPIO_DISK_GEO_016={ 500, 4, 16,  32000 };
mpio_disk_phy_t MPIO_DISK_GEO_032={ 500, 4, 16,  64000 };
mpio_disk_phy_t MPIO_DISK_GEO_064={ 500, 4, 32, 128000 };
mpio_disk_phy_t MPIO_DISK_GEO_128={ 500, 4, 32, 256000 };

/* This comes from the Samsung documentation files */

/* a small warning: only use this function (mpio_id2mem) in the very
 * beginning of MPIO model detection. After that use the values
 * of size and chips! -mager
 */

int
mpio_id2mem(BYTE id)
{
  int i=0;
  
  switch(id) 
    {
    case 0xea:
      i=2;
      break;
    case 0xe3:  /* Samsung */
    case 0xe5:  /* Toshiba */
      i=4;
      break;
    case 0xe6:
      i=8;
      break;
    case 0x73:
      i=16;
      break;
    case 0x75:
      i=32;
      break;
    case 0x76:
      i=64;      
      break;
    case 0x79:
      i=128;
      break;
    default:
      debug("This should never happen (id2mem)!\n");      
      exit (1);
    }
  return i;
}      

BYTE *
mpio_id2manufacturer(BYTE id)
{
  BYTE *m;
  switch(id) 
    {
    case 0xec:
      m="Samsung";      
      break;
    case 0x98:
      m="Toshiba";
      break;
    default:
      m="unknown";
    }  
  return m;
}

BYTE
mpio_id_valid(BYTE id)
{
  switch(id) 
    {
    case 0xec:   /* Samsung */
    case 0x98:   /* Toshiba */
      return 1;      
    default:
      ;
    }  

  return 0;
}

void
mpio_id2geo(BYTE id, mpio_disk_phy_t *geo)
{
  switch(id) 
    {
    case 0xea:
      *geo = MPIO_DISK_GEO_002;
      break;
    case 0xe3:  /* Samsung */
    case 0xe5:  /* Toshiba */
      *geo = MPIO_DISK_GEO_004;
      break;
    case 0xe6:
      *geo = MPIO_DISK_GEO_008;
      break;
    case 0x73:
      *geo = MPIO_DISK_GEO_016;
      break;
    case 0x75:
      *geo = MPIO_DISK_GEO_032;
      break;
    case 0x76:
      *geo = MPIO_DISK_GEO_064;
      break;
    case 0x79:
      *geo = MPIO_DISK_GEO_128;
      break;
    default:
      debug("This should never happen!\n");      
      exit (1);      
    }
  
  return;
}
