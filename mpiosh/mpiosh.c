/* -*- linux-c -*- */

/* 
 *
 * $Id: mpiosh.c,v 1.2 2002/09/13 07:00:46 crunchy Exp $
 *
 * Author: Andreas Büsching  <crunchy@tzi.de>
 *
 * mpiosh - user interface of the mpio library, providing access to
 * 	    some model of the MPIO mp3 players.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * */

#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "libmpio/debug.h"
#include "libmpio/mpio.h"

#include "callback.h"
#include "mpiosh.h"

mpiosh_t mpiosh;

const char *PROMPT_INT = "\033[;1mmpio <i>\033[m ";
const char *PROMPT_EXT = "\033[;1mmpio <e>\033[m ";

mpiosh_cmd_t commands[] = {
  { "debug", "[level|file|on|off] <value>",
    "modify debugging options",
    mpiosh_cmd_debug },
  { "ver", NULL,
    "version of mpio package",
    mpiosh_cmd_version },
  { "help", NULL,
    "show known commands",
    mpiosh_cmd_help },
  { "dir", NULL,
    "list content of current memory card",
    mpiosh_cmd_dir },
  { "info", NULL,
    "show information about MPIO player",
    mpiosh_cmd_info },
  { "mem", "[i|e]",
    "set current memory card. 'i' selects the internal and 'e' "
    "selects the external memory card (smart media card)",
    mpiosh_cmd_mem },
  { "open", NULL,
    "open connect to MPIO player",
    mpiosh_cmd_open },
  { "close", NULL,
    "close connect to MPIO player",
    mpiosh_cmd_close },
  { "quit", " or exit",
    "exit mpiosh and close the device",
    mpiosh_cmd_quit },
  { "exit", NULL, NULL, mpiosh_cmd_quit },
  { "get", "<filename>",
    "read <filename> from memory card",
    mpiosh_cmd_get },
  { "mget", "<regexp>",
    "read all files matching the regular expression "
    "from the selected memory card",
    mpiosh_cmd_mget },
  { "put", "<filename>",
    "write <filename> to memory card",
    mpiosh_cmd_put },
  { "mput", "<regexp>",
    "write all local files matching the regular expression "
    "to the selected memory card",
    mpiosh_cmd_mput },
  { "del", "<filename>",
    "deletes <filename> from memory card",
    mpiosh_cmd_del },
  { "mdel", "<regexp>",
    "deletes all files matching the regular expression "
    "from the selected memory card",
    mpiosh_cmd_mdel },
  { "dump", NULL,
    "get all files of current memory card",
    mpiosh_cmd_dump },
  { "free", NULL,
    "display amount of available bytes of current memory card",
    mpiosh_cmd_free },
  { "format", NULL,
    "format current memory card",
    mpiosh_cmd_format },
  { "switch", "<file1> <file2>",
    "switches the order of two files",
    mpiosh_cmd_switch },
  { "ldir", NULL,
    "list local directory",
    mpiosh_cmd_ldir },
  { "lcd", NULL,
    "change the current working directory",
    mpiosh_cmd_lcd },
  { "lmkdir", NULL,
    "create a local directory",
    mpiosh_cmd_lmkdir },
  { NULL, NULL, NULL, NULL }
};

/* readline extensaions */
void
mpiosh_readline_init(void)
{
  rl_readline_name = "mpio";
  rl_attempted_completion_function = mpiosh_readline_completion;
}

char *
mpiosh_readline_comp_cmd(const char *text, int state)
{
  static mpiosh_cmd_t *cmd = NULL;
  char *cmd_text = NULL;
  
  if (state == 0) {
    cmd = commands;
  }
  
  while (cmd->cmd) {
    if ((*text == '\0') || (strstr(cmd->cmd, text) == cmd->cmd)) {
      cmd_text = strdup(cmd->cmd);
      cmd++;
      break;
    }
    cmd++;
  }

  return cmd_text;
}

char **
mpiosh_readline_completion(const char *text, int start, int end)
{
  char **matches = (char**)NULL;

  UNUSED(end);

  if (start == 0)
    matches = rl_completion_matches(text, mpiosh_readline_comp_cmd);
  else {
    
  }
  
  return matches;
}

/* helper functions */
void
mpiosh_init(void)
{
  mpiosh.dev = NULL;
  mpiosh.card = MPIO_INTERNAL_MEM;
  mpiosh.prompt = PROMPT_INT;
}

char **
mpiosh_command_split(char *line)
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

mpiosh_cmd_t *
mpiosh_command_find(char *line)
{
  mpiosh_cmd_t *cmd = commands;
 
  while (cmd->cmd) {
    if (strstr(line, cmd->cmd) == line) {
      if (line[strlen(cmd->cmd)] == ' ' ||
	  line[strlen(cmd->cmd)] == '\0') 
	return cmd;
    }
    cmd++;
  }

  return NULL;
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


int
main(int argc, char *argv[]) {
  char *	line;
  char **	cmds, **walk;
  mpiosh_cmd_t *cmd;
  
  UNUSED(argc);
  UNUSED(argv);
  
  setenv("mpio_debug", "", 0);
  setenv("mpio_color", "", 0);

  /* no unwanted interruption anymore */
  signal(SIGINT, SIG_IGN);
  
  /* init readline and history */
  rl_readline_name = "mpio";
  using_history();
  
  debug_init();
  mpiosh_readline_init();
  
  mpiosh_init();
  mpiosh.dev = mpio_init();

  if (!mpiosh.dev) {
    printf("could not find MPIO player.\n");
  }

  if (mpiosh.card == MPIO_INTERNAL_MEM)
    mpiosh.prompt = PROMPT_INT;
  else
    mpiosh.prompt = PROMPT_EXT;

  while ((line = readline(mpiosh.prompt))) {
    if (*line == '\0') continue;
    
    cmds = mpiosh_command_split(line);

    if (cmds[0][0] == '\0') {
      free(cmds);
      continue;
    }
    
    walk = cmds;
    while (*walk) {      
      cmd = mpiosh_command_find(*walk);

      if (cmd) {
	char ** help, **args = mpiosh_command_get_args(*walk);
	help = args;
      
	cmd->func(args);
	mpiosh_command_free_args(args);
      } else
	printf("unknown command: '%s'\n", *walk);

/*       if ((idx = history_search(line, -1)) != -1) */
/* 	history_set_pos(idx); */
/*       else */
      add_history(line);
      free(*walk);
      walk++;
    }
    free(cmds);
  }

  mpiosh_cmd_quit(NULL);

  return 0;
}

  
