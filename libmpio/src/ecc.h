/*
 * $Id: ecc.h,v 1.3 2006/01/21 18:33:20 germeier Exp $
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

#ifndef _MPIO_ECC_H_
#define _MPIO_ECC_H_

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 256 Bytes Data, 3 Byte ECC to generate */
int	mpio_ecc_256_gen(CHAR *, CHAR *);
/* 256 Bytes Data, 3 Bytes ECC to check and possibly correct */
int	mpio_ecc_256_check(CHAR *, CHAR*);

#ifdef __cplusplus
}
#endif 

#endif
