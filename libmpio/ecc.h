/* 
 *
 * $Id: ecc.h,v 1.1 2002/08/28 16:10:51 salmoon Exp $
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

#ifndef _MPIO_ECC_H_
#define _MPIO_ECC_H_

#include "defs.h"

/* 256 Bytes Data, 3 Byte ECC to generate */
int	mpio_ecc_256_gen(BYTE *, BYTE *);
/* 256 Bytes Data, 3 Bytes ECC to check and possibly correct */
int	mpio_ecc_256_check(BYTE *, BYTE*);

#endif
