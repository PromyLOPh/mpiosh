/* 
 *
 * $Id: cis.c,v 1.1 2003/04/23 08:34:14 crunchy Exp $
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

#include "cis.h"

/* This data is CIS block (Card Information System) of the SmartMedia
 * cards, it can be found here:
 * http://www.samsungelectronics.com/semiconductors/flash/technical_data/application_notes/SM_format.pdf  (page 8 + 9)
 * or on any Smartmedia card
 *
 * It is not believed to be some kind of trade secret or such, so ...
 * ... if we are mistaken, please tell us so.
 */

char cis_templ[0x80] = {
  0x01, 0x03, 0xd9, 0x01, 0xff, 0x18, 0x02, 0xdf, /* 0x00 */
  0x01, 0x20, 0x04, 0x00, 0x00, 0x00, 0x00, 0x21,

  0x02, 0x04, 0x01, 0x22, 0x02, 0x01, 0x01, 0x22, /* 0x10  */
  0x03, 0x02, 0x04, 0x07, 0x1a, 0x05, 0x01, 0x03,

  0x00, 0x02, 0x0f, 0x1b, 0x08, 0xc0, 0xc0, 0xa1, /* 0x20 */
  0x01, 0x55, 0x08, 0x00, 0x20, 0x1b, 0x0a, 0xc1, 

  0x41, 0x99, 0x01, 0x55, 0x64, 0xf0, 0xff, 0xff, /* 0x30 */
  0x20, 0x1b, 0x0c, 0x82, 0x41, 0x18, 0xea, 0x61,

  0xf0, 0x01, 0x07, 0xf6, 0x03, 0x01, 0xee, 0x1b, /* 0x40 */
  0x0c, 0x83, 0x41, 0x18, 0xea, 0x61, 0x70, 0x01,

  0x07, 0x76, 0x03, 0x01, 0xee, 0x15, 0x14, 0x05, /* 0x50 */
  0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,

  0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x30, 0x2e, /* 0x60 */
  0x30, 0x00, 0xff, 0x14, 0x00, 0xff, 0x00, 0x00,

  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x70 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

};

BYTE *
mpio_cis_gen()
{
  BYTE *p;
  
  p = (BYTE *)malloc(SECTOR_SIZE);
  
  memset(p, 0, SECTOR_SIZE);  
  memcpy(p,       cis_templ, 0x80);
  memcpy(p+0x100, cis_templ, 0x80);
  
  return p;
  
}


  
