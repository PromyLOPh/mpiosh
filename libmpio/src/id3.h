/*
 * $Id: id3.h,v 1.3 2006/01/21 18:33:20 germeier Exp $
 *
 *  libmpio - a library for accessing Digit@lways MPIO players
 *  Copyright (C) 2003 Markus Germeier
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _MPIO_ID3_H_
#define _MPIO_ID3_H_

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ID3 rewriting: do the work */
/* context, src filename, uniq filename template */
int    mpio_id3_do(mpio_t *, CHAR *, CHAR *);  

/* ID3: clean up temp file */
int    mpio_id3_end(mpio_t *);

#ifdef __cplusplus
}
#endif 


#endif /* _MPIO_ID3_H_ */
