/* -*- linux-c -*- */

/* 
 *
 * $Id: smartmedia.h,v 1.1 2002/08/28 16:10:51 salmoon Exp $
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

#ifndef _MPIO_SMARTMEDIA_H_
#define _MPIO_SMARTMEDIA_H_

#include "defs.h"

/* get our configuration for SmartMedia cards right */
int	mpio_id2mem (BYTE);
BYTE *	mpio_id2manufacturer(BYTE);
void	mpio_id2geo(BYTE, mpio_disk_phy_t *);
BYTE	mpio_id_valid(BYTE);

#endif /* _MPIO_SMARTMEDIA_H_ */
