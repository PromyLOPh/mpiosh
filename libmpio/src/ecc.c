/*
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

#include "ecc.h"
#include "debug.h"

#define GET_BIT(d, o) (((d) >> (o)) & 0x01 )
#define XOR_BITS(d, x1, x2, x3, x4) (GET_BIT((d), (x1)) ^ \
                                     GET_BIT((d), (x2)) ^ \
                                     GET_BIT((d), (x3)) ^ \
                                     GET_BIT((d), (x4)))
#define ADD_BITS(c, d1, d2, v) if ((c)) d1 ^= (v); else d2 ^= (v)

int 
mpio_ecc_256_gen(CHAR *data, CHAR *ecc)
{
  CHAR p1, p1_;
  CHAR p2, p2_;
  CHAR p4, p4_;

  CHAR p08, p08_;
  CHAR p16, p16_;
  CHAR p32, p32_;
  CHAR p64, p64_;

  CHAR p0128, p0128_;
  CHAR p0256, p0256_;
  CHAR p0512, p0512_;
  CHAR p1024, p1024_;

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
    p1 ^= XOR_BITS(data[i], 7, 5, 3, 1);
    p1_^= XOR_BITS(data[i], 6, 4, 2, 0);

    /* p2, p2_ */
    p2 ^= XOR_BITS(data[i], 7, 6, 3, 2);
    p2_^= XOR_BITS(data[i], 5, 4, 1, 0);

    /* p4, p4_ */
    p4 ^= XOR_BITS(data[i], 7, 6, 5, 4);
    p4_^= XOR_BITS(data[i], 3, 2, 1, 0);
  }

  /* horizontal */
  for (i=0; i<8; i++) {
    for (j=0; j<256; j++) {  
      /* p08, p08_ */
      ADD_BITS(GET_BIT(j, 0), p08, p08_, GET_BIT(data[j], i));      
      
      /* p16, p16_ */
      ADD_BITS(GET_BIT(j, 1), p16, p16_, GET_BIT(data[j], i));      

      /* p32, p32_ */
      ADD_BITS(GET_BIT(j, 2), p32, p32_, GET_BIT(data[j], i));

      /* p64, p64_ */
      ADD_BITS(GET_BIT(j, 3), p64, p64_, GET_BIT(data[j], i));

      /* p0128, p0128_ */
      ADD_BITS(GET_BIT(j, 4), p0128, p0128_, GET_BIT(data[j], i));
      
      /* p0256, p0256_ */
      ADD_BITS(GET_BIT(j, 5), p0256, p0256_, GET_BIT(data[j], i));

      /* p0512, p0512_ */
      ADD_BITS(GET_BIT(j, 6), p0512, p0512_, GET_BIT(data[j], i));

      /* p1024, p1024_ */
      ADD_BITS(GET_BIT(j, 7), p1024, p1024_, GET_BIT(data[j], i));
    }
  }  

  /* calculate actual ECC */
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
mpio_ecc_256_check(CHAR *data, CHAR *ecc)
{
  CHAR own_ecc[3];
  CHAR check[3];
  CHAR line, col;
  
  int v, i;
  
  mpio_ecc_256_gen(data, own_ecc);

  check[0] = (ecc[0] ^ own_ecc[0]);
  check[1] = (ecc[1] ^ own_ecc[1]);
  check[2] = (ecc[2] ^ own_ecc[2]);

  /* if ECC is wrong, try to fix one-bit errors */
  if (check[0] | check[1] | check[2]) {
    debugn(3, "ECC %2x %2x %2x vs. %2x %2x %2x\n", 
	   ecc[0], ecc[1], ecc[2], own_ecc[0], own_ecc[1], own_ecc[2]);
    
    v=1;
    for(i=0; i<4; i++)
      {
	if (!( (GET_BIT(check[1], i*2) ^
	       (GET_BIT(check[1], i*2+1)))))
	  v=0;
	if (!( (GET_BIT(check[0], i*2) ^
	       (GET_BIT(check[0], i*2+1)))))
	  v=0;
      }
    for(i=1; i<4; i++)
      {
	if (!( (GET_BIT(check[2], i*2) ^
	       (GET_BIT(check[2], i*2+1)))))
	  v=0;
      }
    
    if (v) {
      debugn(2, "correctable error detected ... fixing the bit\n");
      line = 
	GET_BIT(check[1], 7) * 128 +
	GET_BIT(check[1], 5) * 64  +
	GET_BIT(check[1], 3) * 32  +
	GET_BIT(check[1], 1) * 16  +
	GET_BIT(check[0], 7) * 8   +
	GET_BIT(check[0], 5) * 4   +
	GET_BIT(check[0], 3) * 2   +
	GET_BIT(check[0], 1);
      col  = 
	GET_BIT(check[2], 7) * 4 +
	GET_BIT(check[2], 5) * 2 +
	GET_BIT(check[2], 3);
      debugn(3, "error in line: %d , col %d (byte is: %02x)\n", 
	     line, col, data[line]);
      data[line] ^= ( 1 << col);
      debugn(3, "fixed byte is: %02x\n", data[line]);
    } else {				       
      debugn(2, "uncorrectable error detected. Sorry, you lose!\n");
      return 1;
    }
  }
  
  return 0;  
}


  

