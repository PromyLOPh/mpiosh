/* readline.h
 *
 * Author: Andreas Büsching  <crunchy@tzi.de>
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

#ifndef MPIOSH_READLINE_HH
#define MPIOSH_READLINE_HH

#include <stdio.h>

#include <readline/readline.h>
#include <readline/history.h>

/* readline extensions */
void mpiosh_readline_init(void);
char **mpiosh_readline_completion(const char *text, int start, int end);

void mpiosh_readline_noredisplay(void);
void mpiosh_readline_pipe(void);
int mpiosh_readline_cancel(void);

char *mpiosh_readline_comp_cmd(const char *text, int state);
char *mpiosh_readline_comp_onoff(const char *text, int state);
char *mpiosh_readline_comp_mpio_file(const char *text, int state);
char *mpiosh_readline_comp_config(const char *text, int state);

#endif 

/* end of readline.h */
