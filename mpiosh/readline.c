/* readline.c
 *
 * Author: Andreas Büsching  <crunchy@tzi.de>
 *
 * $Id: readline.c,v 1.4 2002/10/18 08:39:23 crunchy Exp $
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

#include "readline.h"

#include "command.h"
#include "mpiosh.h"

/* readline extensions */
void
mpiosh_readline_init(void)
{
  rl_readline_name = "mpio";
  rl_catch_signals = 0;
  rl_completer_quote_characters = "\"\'";
  rl_filename_quote_characters = " \"\' ";
  rl_attempted_completion_function = mpiosh_readline_completion;
  rl_event_hook = mpiosh_readline_cancel;
}

char *
mpiosh_readline_comp_cmd(const char *text, int state)
{
  static mpiosh_cmd_t *	cmd = NULL;
  static char **	alias = NULL;
  
  char *cmd_text = NULL;
  
  if (state == 0) {
    cmd = commands;
  }
  
  while (cmd->cmd) {
    if (!alias) {
      if ((*text == '\0') || (strstr(cmd->cmd, text) == cmd->cmd)) {
	cmd_text = strdup(cmd->cmd);
	if (cmd->aliases) alias = cmd->aliases;
	else cmd++;
	break;
      }
      if (cmd->aliases) alias = cmd->aliases;
      else cmd++;
    } else {
      int break_it = 0;

      while (*alias) {
	if (strstr(*alias, text) == *alias) {
	  cmd_text = strdup(*alias);
	  alias++;
	  break_it = 1;
	  break;
	}
	alias++;
      }
      if (break_it) break;
      if (*alias == NULL) cmd++, alias = NULL;
    }
  }

  return cmd_text;
}

char *
mpiosh_readline_comp_mpio_file(const char *text, int state)
{
  static BYTE *p;
  char *arg = NULL;
  BYTE month, day, hour, minute;
  char fname[100];
  WORD year;  
  DWORD fsize;  

  if (mpiosh.dev == NULL) {
    rl_attempted_completion_over = 1;
    return NULL;
  }
  
  if (state == 0) p = mpio_directory_open(mpiosh.dev, mpiosh.card);

  while ((p != NULL) && (arg == NULL)) {
    memset(fname, '\0', 100);
    
    mpio_dentry_get(mpiosh.dev, mpiosh.card, p,
		    fname, 100,
		    &year, &month, &day,
		    &hour, &minute, &fsize);

    if (strstr(fname, text) == fname) {
      arg = strdup(fname);
      if (strchr(arg, ' ')) {
	rl_filename_completion_desired = 1;
	rl_filename_quoting_desired = 1;
      }
    }

    p = mpio_dentry_next(mpiosh.dev, mpiosh.card, p);
  }
  
  
  return arg;
}

char *
mpiosh_readline_comp_config(const char *text, int state)
{
  char *arg = NULL;

  return arg;
}


char **
mpiosh_readline_completion(const char *text, int start, int end)
{
  char **matches = (char**)NULL;

  UNUSED(end);

  if (start == 0) /* command completion */
    matches = rl_completion_matches(text, mpiosh_readline_comp_cmd);
  else {
    mpiosh_cmd_t	*scmd;
    char		*cmd, *help= rl_line_buffer;

    while (*help != ' ') help++;
    cmd = malloc(help - rl_line_buffer + 1);
    strncpy(cmd, rl_line_buffer, help - rl_line_buffer + 1);
    cmd[help - rl_line_buffer] = '\0';
    
    if ((scmd = mpiosh_command_find(cmd))) {
      if (scmd->comp_func) {
	matches = rl_completion_matches(text, scmd->comp_func);
      }
    }
    
    free(cmd);
  }
  
  return matches;
}

int
mpiosh_readline_cancel(void)
{
  if (mpiosh_cancel) rl_done = 1;
  
  return 0;
}

void
mpiosh_readline_noredisplay(void)
{
}

void
mpiosh_readline_pipe(void)
{
  rl_redisplay_function = mpiosh_readline_noredisplay;
  rl_event_hook = NULL;
}

/* end of readline.c */
