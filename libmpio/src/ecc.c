/* 
 *
 * $Id: ecc.c,v 1.1 2003/04/23 08:34:14 crunchy Exp $
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
  BYTE check[3];
  BYTE c;
  BYTE line, col;
  
  int v, i;
  
  mpio_ecc_256_gen(data, own_ecc);
  if ((own_ecc[0]^ecc[0])|
      (own_ecc[1]^ecc[1])|
      (own_ecc[2]^ecc[2])) {
    debugn(2, "ECC %2x %2x %2x vs. %2x %2x %2x\n", 
	  ecc[0], ecc[1], ecc[2], own_ecc[0], own_ecc[1], own_ecc[2]);
    check[0] = (ecc[0] ^ own_ecc[0]);
    check[1] = (ecc[1] ^ own_ecc[1]);
    check[2] = (ecc[2] ^ own_ecc[2]);
    
    v=1;
    for(i=0; i<4; i++)
      {
	if (!((get_bit(check[1], i*2) ^
	       (get_bit(check[1], i*2+1)))))
	  v=0;
	if (!((get_bit(check[0], i*2) ^
	       (get_bit(check[0], i*2+1)))))
	  v=0;
      }
    for(i=1; i<4; i++)
      {
	if (!((get_bit(check[2], i*2) ^
	       (get_bit(check[2], i*2+1)))))
	  v=0;
      }
    
    if (v) {
      debug("correctable error detected ... fixing the bit\n");
      line = get_bit(check[1], 7) * 128 +
	get_bit(check[1], 5) * 64  +
	get_bit(check[1], 3) * 32  +
	get_bit(check[1], 1) * 16  +
	get_bit(check[0], 7) * 8   +
	get_bit(check[0], 5) * 4   +
	get_bit(check[0], 3) * 2   +
	get_bit(check[0], 1);
      col  = get_bit(check[2], 7) * 4 +
	get_bit(check[2], 5) * 2 +
	get_bit(check[2], 3);
      debug ("error in line: %d , col %d\n", line, col);
      debug ("defect byte is: %02x\n", data[line]);
      data[line] ^= ( 1 << col);
      debug ("fixed  byte is: %02x\n", data[line]);
    } else {				       
      debug("uncorrectable error detected. Sorry you lose!\n");
      return 1;
    }
  }
  
  return 0;  
}


  

