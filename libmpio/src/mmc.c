/*
 * $Id: mmc.c,v 1.1 2004/05/30 16:28:52 germeier Exp $
 *
 *  libmpio - a library for accessing Digit@lways MPIO players
 *  Copyright (C) 2004 Robert Kellington, Markus Germeier
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

#include <stdio.h>
#include <stdlib.h>

#include "mmc.h"
#include "debug.h"

/*
  This routine attempts to detect when mmc module is present and set some variables according to 
  that size. If adding a new memory module do the following changes:
  1. Add the appropriate switch below (size and geo need to be set).
  2. Add the new module size and array data into the routines mmc_mpio_mbr_gen and mmc_mpio_pbr_gen for the memory 
  formating.
  That should be all
*/    
BYTE mpio_mmc_detect_memory(mpio_t *m, mpio_mem_t mem)
{
  mpio_smartmedia_t *sm=0;
  int size;

  /* just in case we encounter a player with an internel mmc memory */
  if (mem == MPIO_INTERNAL_MEM) sm = &m->internal;
  if (mem == MPIO_EXTERNAL_MEM) sm = &m->external;
  if (!sm)
    {
      debug("error in memory selection, aborting\n");      
      exit (-1);
    }
  
  /*looking for likely markers*/
  /*
    if((m->version[0x28] == 0x32) && (m->version[0x3a] == 0x01) && (m->version[0x3b] == 0x01) && 
    (m->version[0x30] == 0xff) && (m->version[0x31] == 0xcf))
  */   
  if((m->version[0x39] == 0x01) && (m->version[0x3b] == 0x01) && (m->version[0x20] == 0xff) && (m->version[0x21] == 0xff) &&
     (m->version[0x22] == 0xff) && (m->version[0x23] == 0xff) && (m->version[0x24] == 0xff) && (m->version[0x25] == 0xff))
    {
      switch(m->version[0x26])
	{
	case 0x0e:    /* 16 Mb */
	  /*NOTE: This size needs formating to be added and testing*/
	  sm->size = 16;
	  sm->geo  = MPIO_DISK_GEO_016;
	  break;
	case 0x1e:    /* 32 Mb */
	  sm->size = 32;
	  sm->geo  = MPIO_DISK_GEO_032;
	  break;
	case 0x3b:    /* 64 Mb */
	  sm->size = 64;
	  sm->geo  = MPIO_DISK_GEO_064;
	  break;
	case 0x78:    /* 128 Mb */
	  sm->size = 128;
	  sm->geo  = MPIO_DISK_GEO_128;
	  break;
	case 0xf1:    /* 256 Mb */
	  sm->size = 256;
	  sm->geo  = MPIO_DISK_GEO_256;
	  break;
	default:
	  debug("Found MMC memory markers but didn't recognize size\n");      
	  sm->mmc  = 0;
	  sm->size = 0;
	  return 0;
	}
      sm->mmc = 1;
      sm->chips = 1;
      sm->version = 0;
      sm->id = 0x78;        /* Assigning my random id */
    
      debug("Found MMC memory and all is OK so far\n");      

      return 1;
    }

  debug("Not recognized MMC memory\n");
 
  return 0;
}
