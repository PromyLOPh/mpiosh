/* mpiosh.h
 *
 * Author: Andreas Büsching  <crunchy@tzi.de>
 *
 * $Id: mpiosh.h,v 1.4 2002/09/14 09:55:31 crunchy Exp $
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

typedef void(*cmd_callback)(char *args[]);
  
typedef struct {
  mpio_t *	dev;
  mpio_mem_t 	card;
  const char *	prompt;
} mpiosh_t;
  
typedef struct {
  char *	cmd;
  char *	args;
  char *	info;
  cmd_callback	func;
} mpiosh_cmd_t;
  
/* signal handler */
void mpiosh_signal_handler(int signal);

/* readline extensions */
void mpiosh_readline_init(void);
char **mpiosh_readline_completion(const char *text, int start, int end);
char *mpiosh_readline_comp_cmd(const char *text, int state);
int mpiosh_readline_cancel(void);

/* helper functions */
void mpiosh_init(void);
mpiosh_cmd_t *mpiosh_command_find(char *line);
char **mpiosh_command_split(char *line);
char **mpiosh_command_get_args(char *line);
void mpiosh_command_free_args(char **args);

/* global structures */
extern mpiosh_t mpiosh;
extern mpiosh_cmd_t commands[];
extern int mpiosh_cancel;

extern const char *PROMPT_INT;
extern const char *PROMPT_EXT;

#endif // _MPIOSH_H_

/* end of mpiosh.h */
