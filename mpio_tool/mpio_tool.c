/* -*- linux-c -*- */

/* 
 *
 * $Id: mpio_tool.c,v 1.1 2002/08/28 16:10:51 salmoon Exp $
 *
 * Test tool for USB MPIO-*
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "libmpio/mpio.h"
#include "libmpio/debug.h"

#define FOOBAR (64*1024*1024)

BYTE buffer[FOOBAR];
BYTE *p, *q;

mpio_t *test;

BYTE callback(int, int);

BYTE callback(int part, int total)
{
  UNUSED(part);
  UNUSED(total);
  
/*   printf("a: %d\n", a); */
  return 0;
  
}



int
main(int argc, char *argv[]) {
  int debug=1;
  int size;  
  int free, mem;
  
  WORD year;  
  BYTE month, day, hour, minute;  
  DWORD fsize;  
  BYTE fname[100];  

  size=0;
  
  if (argc == 2) {
    debug=atoi(argv[1]);    
  }

  
  test = mpio_init();
  
  if (!test) {
    printf ("Error initializing MPIO library\n");
    exit(1);
  }
  
  /* do stuff */
  
  mem=mpio_memory_free(test, MPIO_INTERNAL_MEM, &free);
  printf("\nInternal Memory (size: %dKB / avail: %dKB):\n", mem/1000, free);
  p=mpio_directory_open(test, MPIO_INTERNAL_MEM);
  while (p!=NULL) {

    mpio_dentry_get(test, p,                   
		    fname, 100,
		    &year, &month, &day,
		    &hour, &minute, &fsize);     

    printf ("%02d.%02d.%04d  %02d:%02d    %9d - %s\n", 
	    day, month, year, hour, minute, fsize, fname);

    p=mpio_dentry_next(test, p);
  }

   if ((mem=mpio_memory_free(test, MPIO_EXTERNAL_MEM, &free))!=0) {     
  
     printf("\nExternal Memory (size: %dKB / avail: %dKB):\n", mem/1000, free);  
    p=mpio_directory_open(test, MPIO_EXTERNAL_MEM);
    while (p!=NULL) {
      
      mpio_dentry_get(test, p,                   
		      fname, 100,
		      &year, &month, &day,
		      &hour, &minute, &fsize);     
      
      printf ("%02d.%02d.%04d  %02d:%02d    %9d - %s\n", 
	      day, month, year, hour, minute, fsize, fname);
      
      p=mpio_dentry_next(test, p);
    } 
  }else {
    printf("\nNo external Memory available!\n");
  }


/*    mpio_file_get(test, MPIO_EXTERNAL_MEM, "Alanis_Morissette_01_21_Things_simplemp3s.mp3", callback); */

  
  mpio_close(test);

  exit(0);
}

  
