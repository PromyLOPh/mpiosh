/* command.h
 *
 * Author: Andreas Buesching  <crunchy@tzi.de>
 *
 * $Id: command.h,v 1.2 2002/10/29 20:03:35 crunchy Exp $
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

#ifndef MPIOSH_COMMAND_HH
#define MPIOSH_COMMAND_HH

#include "mpiosh.h"

/* command(-line) functions */
struct mpiosh_cmd_t *mpiosh_command_find(char *line);
char **mpiosh_command_split_line(char *line);
char **mpiosh_command_get_args(char *line);
void mpiosh_command_regex_fix(char *argv[]);
void mpiosh_command_free_args(char **args);

#endif 

/* end of command.h */
