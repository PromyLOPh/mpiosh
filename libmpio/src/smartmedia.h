/*
 * $Id: smartmedia.h,v 1.3 2003/07/24 16:17:30 germeier Exp $
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

#ifndef _MPIO_SMARTMEDIA_H_
#define _MPIO_SMARTMEDIA_H_

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* get our configuration for SmartMedia cards right */
int	mpio_id2mem (BYTE);
BYTE *	mpio_id2manufacturer(BYTE);
void	mpio_id2geo(BYTE, mpio_disk_phy_t *);
BYTE	mpio_id_valid(BYTE);
BYTE    mpio_id2version(BYTE);  

#ifdef __cplusplus
}
#endif 

#endif /* _MPIO_SMARTMEDIA_H_ */
