/* 
 *
 * $Id: ecc.c,v 1.1 2002/08/28 16:10:51 salmoon Exp $
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

#include "ecc.h"
#include "debug.h"

BYTE get_bit(BYTE, int);

/* TODO: fix correctable errors */

inline BYTE 
get_bit(BYTE d, int offset)
{
  return ((d >> offset) & 0x01 );
}

int 
mpio_ecc_256_gen(BYTE *data, BYTE *ecc)
{
  BYTE p1, p1_;
  BYTE p2, p2_;
  BYTE p4, p4_;

  BYTE p08, p08_;
  BYTE p16, p16_;
  BYTE p32, p32_;
  BYTE p64, p64_;

  BYTE p0128, p0128_;
  BYTE p0256, p0256_;
  BYTE p0512, p0512_;
  BYTE p1024, p1024_;

  int i, j;

  /* init */
  p1=p1_=0;
  p2=p2_=0;
  p4=p4_=0;

  p08=p08_=0;
  p16=p16_=0;
  p32=p32_=0;
  p64=p64_=0;

  p0128=p0128_=0;
  p0256=p0256_=0;
  p0512=p0512_=0;
  p1024=p1024_=0;

  /* vertical */
  for (i=0; i<256; i++) {
    
    /* p1, p1_ */
    p1 ^= (get_bit(data[i], 7) ^
	   get_bit(data[i], 5) ^
	   get_bit(data[i], 3) ^
	   get_bit(data[i], 1));
    p1_^= (get_bit(data[i], 6) ^
	   get_bit(data[i], 4) ^
	   get_bit(data[i], 2) ^
	   get_bit(data[i], 0));

    /* p2, p2_ */
    p2 ^= (get_bit(data[i], 7) ^
	   get_bit(data[i], 6) ^
	   get_bit(data[i], 3) ^
	   get_bit(data[i], 2));
    p2_^= (get_bit(data[i], 5) ^
	   get_bit(data[i], 4) ^
	   get_bit(data[i], 1) ^
	   get_bit(data[i], 0));

    /* p4, p4_ */
    p4 ^= (get_bit(data[i], 7) ^
	   get_bit(data[i], 6) ^
	   get_bit(data[i], 5) ^
	   get_bit(data[i], 4));
    p4_^= (get_bit(data[i], 3) ^
	   get_bit(data[i], 2) ^
	   get_bit(data[i], 1) ^
	   get_bit(data[i], 0));
  }

  /* horizontal */
  for (i=0; i<8; i++) {
    
    for (j=0; j<256; j++) {
  
      /* p08, p08_ */
      if ((j & 0x01) == 0) 
	p08_^= get_bit(data[j], i);
      if ((j & 0x01) == 1) 
	p08 ^= get_bit(data[j], i);
      
      /* p16, p16_ */
      if (((j/2) & 0x01) == 0)
	p16_^= get_bit(data[j], i);
      if (((j/2) & 0x01) == 1)
	p16^= get_bit(data[j], i);

      /* p32, p32_ */
      if (((j/4) & 0x01) == 0)
	p32_^= get_bit(data[j], i);
      if (((j/4) & 0x01) == 1)
	p32^= get_bit(data[j], i);

      /* p64, p64_ */
      if (((j/8) & 0x01) == 0)
	p64_^= get_bit(data[j], i);
      if (((j/8) & 0x01) == 1)
	p64^= get_bit(data[j], i);


      /* p0128, p0128_ */
      if (((j/16) & 0x01) == 0) 
	p0128_^= get_bit(data[j], i);
      if (((j/16) & 0x01) == 1) 
	p0128 ^= get_bit(data[j], i);
      
      /* p0256, p0256_ */
      if (((j/32) & 0x01) == 0)
	p0256_^= get_bit(data[j], i);
      if (((j/32) & 0x01) == 1)
	p0256^= get_bit(data[j], i);

      /* p0512, p0512_ */
      if (((j/64) & 0x01) == 0)
	p0512_^= get_bit(data[j], i);
      if (((j/64) & 0x01) == 1)
	p0512^= get_bit(data[j], i);

      /* p1024, p1024_ */
      if (((j/128) & 0x01) == 0)
	p1024_^= get_bit(data[j], i);
      if (((j/128) & 0x01) == 1)
	p1024^= get_bit(data[j], i);

    }
  }  

  ecc[0]=~((p64 << 7) | (p64_ << 6) | 
	   (p32 << 5) | (p32_ << 4) |
	   (p16 << 3) | (p16_ << 2) |
	   (p08 << 1) | (p08_ << 0));  

  ecc[1]=~((p1024 << 7) | (p1024_ << 6) | 
	   (p0512 << 5) | (p0512_ << 4) |
	   (p0256 << 3) | (p0256_ << 2) |
	   (p0128 << 1) | (p0128_ << 0));  

  ecc[2]=~((p4 << 7) | (p4_ << 6) | 
	   (p2 << 5) | (p2_ << 4) |
	   (p1 << 3) | (p1_ << 2));  

  return 0;
}


int
mpio_ecc_256_check(BYTE *data, BYTE *ecc)
{
  BYTE own_ecc[3];
  mpio_ecc_256_gen(data, own_ecc);
  if ((own_ecc[0]^ecc[0])|
      (own_ecc[1]^ecc[1])|
      (own_ecc[2]^ecc[2])) {
    debug("ECC %2x %2x %2x vs. %2x %2x %2x\n", 
	  ecc[0], ecc[1], ecc[2], own_ecc[0], own_ecc[1], own_ecc[2]);
    debugn(2, "ECC Error detected\n");
    debugn(2, "ECC Correction code not in place yet, Sorry\n");
    debugn(3, "got   ECC: %2x %2x %2x\n", ecc[0], ecc[1], ecc[2]);
    debugn(3, "calc. ECC: %2x %2x %2x\n", own_ecc[0], own_ecc[1], own_ecc[2]);
  }
  
  return 0;  
}


  

