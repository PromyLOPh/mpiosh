/* global.h
 *
 * Author: Andreas Buesching  <crunchy@tzi.de>
 *
 * $Id: global.h,v 1.1 2002/10/12 20:06:22 crunchy Exp $
 *
 * Copyright (C) 2001 Andreas Büsching <crunchy@tzi.de>
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

#ifndef MPIOSH_GLOBAL_HH
#define MPIOSH_GLOBAL_HH

#include "libmpio/mpio.h"

/* type definitions */
typedef void(*mpiosh_cmd_callback_t)(char *args[]);
typedef char *(*mpiosh_comp_callback_t)(const char *text, int state);
  
typedef struct {
  mpio_t *	dev;
  mpio_mem_t 	card;
  const char *	prompt;
} mpiosh_t;
  
typedef struct {
  char *	cmd;
  char **	aliases;
  char *	args;
  char *	info;
  mpiosh_cmd_callback_t		cmd_func;
  mpiosh_comp_callback_t	comp_func;
} mpiosh_cmd_t;
  
/* global structures */
extern mpiosh_t mpiosh;
extern mpiosh_cmd_t commands[];
extern int mpiosh_cancel;
extern int mpiosh_cancel_ack;

extern const char *PROMPT_INT;
extern const char *PROMPT_EXT;

#endif 

/* end of global.h */
