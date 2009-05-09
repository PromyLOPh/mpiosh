/* mpiosh.h
 *
 * Author: Andreas Büsching  <crunchy@tzi.de>
 *
 * $Id: mpiosh.h,v 1.8 2002/10/12 20:06:22 crunchy Exp $
 *
 * Copyright (C) 2002 Andreas Büsching <crunchy@tzi.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _MPIOSH_H_
#define _MPIOSH_H_

#include "libmpio/debug.h"

#include "global.h"

/* mpiosh core functions */
void mpiosh_signal_handler(int signal);
void mpiosh_init(void);

#endif // _MPIOSH_H_

/* end of mpiosh.h */
