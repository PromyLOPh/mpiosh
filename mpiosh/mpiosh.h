/* mpiosh.h
 *
 * Author: Andreas Büsching  <crunchy@tzi.de>
 *
 * $Id: mpiosh.h,v 1.7 2002/10/12 18:31:45 crunchy Exp $
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

#include "libmpio/mpio.h"
#include "libmpio/debug.h"

typedef void(*mpiosh_cmd_callback_t)(char *args[]);
typedef char *(*mpiosh_comp_callback_t)(const char *text, int state);
  
typedef struct {
  mpio_t *	dev;
  mpio_mem_t 	card;
  const char *	prompt;
} mpiosh_t;
  
typedef struct {
  char *	cmd;
  char *	args;
  char *	info;
  mpiosh_cmd_callback_t		cmd_func;
  mpiosh_comp_callback_t	comp_func;
} mpiosh_cmd_t;
  
/* mpiosh core functions */
void mpiosh_signal_handler(int signal);
void mpiosh_init(void);

/* global structures */
extern mpiosh_t mpiosh;
extern mpiosh_cmd_t commands[];
extern int mpiosh_cancel;
extern int mpiosh_cancel_ack;

extern const char *PROMPT_INT;
extern const char *PROMPT_EXT;

#endif // _MPIOSH_H_

/* end of mpiosh.h */
