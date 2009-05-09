/* command.c
 *
 * Author: Andreas Buesching  <crunchy@tzi.de>
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

#include "command.h"

char **
mpiosh_command_split_line(char *line)
{
  char **cmds, *cmd;
  int count = 1;
  char *help, *copy = strdup(line);
  
  help = copy;
  
  while (*help)
    if ((*help++ == ';') && (*(help) != '\0')) count++;
  cmds = malloc(sizeof(char *) * (count + 1));

  cmd = help = copy, count = 0;
  while (*cmd == ' ') cmd++;
  while (help) {
    if (*help == '"') {
      help++;
      while (*help != '\0' && *help != '"') help++;
    }
  
    if (*help == '\0') break;
    if (*help == ';') {
      *help++ = '\0';
      if (*help == '\0') break;
      cmds[count++] = strdup(cmd);
      while (*help == ' ') help++;
      cmd = help;
    }
    help++;
  }
  if (cmd != '\0') {
    cmds[count++] = strdup(cmd);
  }

  cmds[count] = NULL;
  free(copy);
  
  return cmds;
}

struct mpiosh_cmd_t *
mpiosh_command_find(char *line)
{
  struct mpiosh_cmd_t *cmd = commands;
 
  while (cmd->cmd) {
    if (strstr(line, cmd->cmd) == line) {
      if (line[strlen(cmd->cmd)] == ' ' ||
	  line[strlen(cmd->cmd)] == '\0') 
	return cmd;
    } else if (cmd->aliases) {
      char **go = cmd->aliases;
      while (*go) {
	if ((strstr(line, *go) == line) &&
	    ((line[strlen(*go)] == ' ') || (line[strlen(*go)] == '\0'))) {
	  return cmd;
	}
	go++;
      }
    }
    
    cmd++;
  }

  return NULL;
}

void
mpiosh_command_regex_fix(char *argv[])
{
  char **walk = argv;
  char *new_pos, *help;
  char buffer[512];
  
  while (*walk) {
    memset(buffer, '\0', 512);
    help = *walk;
    new_pos = buffer;
    *new_pos++ = '^';
    while (*help != '\0') {
      if (*help == '*' && ((help == *walk) || (*(help - 1) != '.'))) {
	*new_pos++ = '.';
	*new_pos = *help;
      } else if ((*help == '.') && (*(help + 1) != '*')) {
	*new_pos++ = '\\';
	*new_pos = *help;
      } else if (*help == '?' && ((help == *walk) || (*(help - 1) != '\\'))) {
	*new_pos = '.';
      } else {
	*new_pos = *help;
      }
      
      help++, new_pos++;
    }
    *new_pos = '$';
    free(*walk);
    *walk = strdup(buffer);
    
    walk++;
  }
}

char **
mpiosh_command_get_args(char *line)
{
  char **args;
  char *arg_start, *copy, *help, *prev;
  int count = 0, i = 0, go = 1, in_quote = 0;
  
  copy = strdup(line);
  arg_start = strchr(copy, ' ');

  if (arg_start == NULL) {
    args = malloc(sizeof(char *));
    args[0] = NULL;
    return args;
  }
  
  while (*arg_start == ' ') arg_start++;
  
  help = arg_start;
  while (help <= (copy + strlen(copy))) {
    if (*help == '"') {
      help++;count++;
      
      while (*help != '\0' && *help != '"') 
	help++;
      help++;
      while (*help == ' ') help++;
      if (*help == '\0') break;
      in_quote = 1;
    } else if (((help > arg_start) && (*help == '\0')) ||
	(*help == ' ' && (*(help + 1) != '\0') && (*(help + 1) != ' '))) {
      count++;
      in_quote = 0;
      help++;
    } else
      help++;
  }
  
  args = malloc(sizeof(char *) * (count + 1));
  
  help = prev = arg_start;
  in_quote = 0;
  while (go) {
    if (*help == '"') in_quote = !in_quote, help++;
    if (((*help == ' ') && !in_quote) || (in_quote && *help == '"') ||
	(*help == '\0')) {
      if (*help == '\0') {
	go = 0;
	if (*prev == '\0') break;
      }
      
      if (*prev == '"') {
	if (*(help - 1) == '"')
	  *(help - 1) = '\0';
	else
	  *help = '\0';
	
	args[i++] = strdup(prev + 1);
      } else {
	*help = '\0';
	args[i++] = strdup(prev);
      }
      
      if (go) {
	help++;
	if (in_quote) {
	  while (*help != '"') help++;
	  help++;
	  in_quote = 0;
	} else
	  while (*help == ' ') help++;
	prev = help;
      }
    } else
      help++;
  }
  args[i] = NULL;

  free(copy);
  
  return args;
}

void
mpiosh_command_free_args(char **args)
{
  char **arg = args;
  
  while (*arg) free(*arg++);

  free(args);
}

/* end of command.c */
