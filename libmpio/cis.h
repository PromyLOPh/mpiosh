/* 
 *
 * $Id: cis.h,v 1.2 2002/10/26 13:07:42 germeier Exp $
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

#ifndef _MPIO_CIS_H_
#define _MPIO_CIS_H_

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif 

/* generate and return a fresh CIS block */
BYTE	*mpio_cis_gen();

#ifdef __cplusplus
}
#endif 

#endif
