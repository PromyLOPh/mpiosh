/* mpiosh.h
 *
 * Author: Andreas Büsching  <crunchy@tzi.de>
 *
 * $Id: mpiosh.h,v 1.3 2002/09/01 18:27:49 crunchy Exp $
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

#if !defined TRUE
#	define TRUE 1
#endif

#if !defined FALSE
#	define FALSE 1
#endif

typedef void(*cmd_callback)(char *args[]);
typedef enum { NO, YES } bool;
  
typedef struct {
  mpio_t *	dev;
  mpio_mem_t 	card;
  const char *	prompt;
} mpiosh_t;
  
typedef struct {
  char *	cmd;
  cmd_callback	func;
  bool		args;
} mpiosh_cmd_t;
  
const char*	PROMPT_INT = "\033[;1mmpio <i>\033[m ";
const char*	PROMPT_EXT = "\033[;1mmpio <e>\033[m ";

/* readline extensions */
void mpiosh_readline_init(void);
char **mpiosh_readline_completion(const char *text, int start, int end);
char *mpiosh_readline_comp_cmd(const char *text, int state);

/* helper functions */
void mpiosh_init(void);
mpiosh_cmd_t *mpiosh_command_find(char *line);
char **mpiosh_command_get_args(char *line);
void mpiosh_command_free_args(char **args);

/* command callbacks */
void mpiosh_cmd_debug(char *args[]);
void mpiosh_cmd_version(char *args[]);
void mpiosh_cmd_help(char *args[]);
void mpiosh_cmd_dir(char *args[]);
void mpiosh_cmd_info(char *args[]);
void mpiosh_cmd_mem(char *args[]);
void mpiosh_cmd_open(char *args[]);
void mpiosh_cmd_close(char *args[]);
void mpiosh_cmd_quit(char *args[]);
void mpiosh_cmd_get(char *args[]);
void mpiosh_cmd_put(char *args[]);
void mpiosh_cmd_del(char *args[]);
void mpiosh_cmd_dump(char *args[]);
void mpiosh_cmd_free(char *args[]);
void mpiosh_cmd_format(char *args[]);
void mpiosh_cmd_switch(char *args[]);

void mpiosh_cmd_ldir(char *args[]);
void mpiosh_cmd_lcd(char *args[]);
void mpiosh_cmd_lmkdir(char *args[]);

/* progress callbacks */
BYTE mpiosh_callback_get(int read, int total);
BYTE mpiosh_callback_put(int read, int total);
BYTE mpiosh_callback_del(int read, int total);
BYTE mpiosh_callback_format(int read, int total);

#endif // _MPIOSH_H_

/* end of mpiosh.h */
