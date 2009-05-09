/*
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

#ifndef _MPIO_MMC_H_
#define _MPIO_MMC_H_

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

extern mpio_disk_phy_t MPIO_DISK_GEO_001;
extern mpio_disk_phy_t MPIO_DISK_GEO_002;
extern mpio_disk_phy_t MPIO_DISK_GEO_004;
extern mpio_disk_phy_t MPIO_DISK_GEO_008;
extern mpio_disk_phy_t MPIO_DISK_GEO_016;
extern mpio_disk_phy_t MPIO_DISK_GEO_032;
extern mpio_disk_phy_t MPIO_DISK_GEO_064;
extern mpio_disk_phy_t MPIO_DISK_GEO_128;
extern mpio_disk_phy_t MPIO_DISK_GEO_256;

/* get our configuration for MMC cards right */
BYTE  mpio_mmc_detect_memory(mpio_t *m, mpio_mem_t mem);

#ifdef __cplusplus
}
#endif 

#endif /* _MPIO_MMC_H_ */
