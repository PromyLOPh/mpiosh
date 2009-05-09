/*
 *  libmpio - a library for accessing Digit@lways MPIO players
 *  Copyright (C) 2002, 2003 Dirk Meyer  <dmeyer@tzi.de> and
 *                           Andreas Büsching <crunchy@tzi.de>
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

#ifndef _MPIO_DEBUG_H_
#define _MPIO_DEBUG_H_

// if DPACKAGE is not definied use PACKAGE or "unknown"

#ifndef DPACKAGE

#ifdef PACKAGE
#define DPACKAGE PACKAGE
#else
#define DPACKAGE "unknown"
#endif

#endif

#include <stdio.h>

#ifdef sun
#include <sys/int_types.h>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PBOOL
#define PBOOL(x) (x)?"True":"False"
#endif

#define UNUSED(x) (x = x)

#define debugn(n,args...) \
   _debug_n(DPACKAGE, n, __FILE__, __LINE__, __FUNCTION__, args)

#define debug(args...) \
   _debug(DPACKAGE, __FILE__, __LINE__, __FUNCTION__, args)

#define debug_error(fatal,args...) \
   _error(DPACKAGE, __FILE__, __LINE__, __FUNCTION__, fatal, args)

#define hexdump(data,len) \
   _hexdump(DPACKAGE, __FILE__, __LINE__, __FUNCTION__, data, len)

#define hexdumpn(n,data,len) \
  _hexdump_n(DPACKAGE, n, __FILE__, __LINE__, __FUNCTION__, data, len)

#define hexdump_text(text,data,len) \
   _hexdump_text(text, DPACKAGE, __FILE__, __LINE__, __FUNCTION__, data, len)

#define octetstrdump(data,len) \
   _octetstr(DPACKAGE, __FILE__, __LINE__, __FUNCTION__, data, len)

#define ipadr_dump(data,len,text) \
   _octetstr(DPACKAGE, __FILE__, __LINE__, __FUNCTION__, data, len, text)

#define use_debug() \
   use_debug(DPACKAGE)


#define with_special_debug(what) \
   _use_debug(DPACKAGE, what)

void debug_init(void);
int debug_file(char *filename);
int debug_level(int level);
int debug_level_get(void);

void _debug(const char *package, const char* file, int line, 
	    const char* function, const char *format, ...);

void _debug_n(const char *package, const int n, const char* file, 
	      int line, const char* function, const char *format, ...);

void _hexdump (const char *package, const char* file, int line, 
	       const char* function, const char* data, int len);

void _hexdump_n (const char *package, const int n, const char* file, int line, 
		 const char* function, const char* data, int len);

void _hexdump_text (const char *text, 
		    const char *package, const char* file, int line, 
		    const char* function, const char* data, int len);

void _error(const char *package, const char* file, int line, 
	    const char* function, int fatal, const char *format, ...);

void _octetstr(const char *package, const char* file, int line, 
	       const char* function, const uint8_t *str, 
	       const unsigned int len, const char *what);

int _use_debug(int level);

#ifdef __cplusplus
}
#endif 

#endif /* _MPIO_DEBUG_H_ */

/* end of debug.h */
