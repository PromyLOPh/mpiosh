/* -*- linux-c -*- */

/* 
 *
 * $Id: mpiosh.c,v 1.19 2002/10/12 18:31:45 crunchy Exp $
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
#include <sys/types.h>
#include <unistd.h>

#include "libmpio/debug.h"
#include "libmpio/mpio.h"

#include "callback.h"
#include "command.h"
#include "readline.h"
#include "mpiosh.h"

/* structure containing current state */
mpiosh_t mpiosh;

/* flag indicating a user-interrupt of the current command */
int mpiosh_cancel = 0;
int mpiosh_cancel_ack = 0;

/* prompt strings */
const char *PROMPT_INT = "\033[;1mmpio <i>\033[m ";
const char *PROMPT_EXT = "\033[;1mmpio <e>\033[m ";

mpiosh_cmd_t commands[] = {
  { "debug", "[level|file|on|off] <value>",
    "  modify debugging options",
    mpiosh_cmd_debug, NULL },
  { "ver", NULL,
    "  version of mpio package",
    mpiosh_cmd_version, NULL },
  { "help", "[<command>]",
    "  show information about known commands or just about <command>",
    mpiosh_cmd_help, NULL },
  { "dir", NULL,
    "  list content of current memory card",
    mpiosh_cmd_dir, NULL },
  { "info", NULL,
    "  show information about MPIO player",
    mpiosh_cmd_info, NULL },
  { "mem", "[i|e]",
    "  set current memory card. 'i' selects the internal and 'e'\n"
    "  selects the external memory card (smart media card)",
    mpiosh_cmd_mem, NULL },
  { "open", NULL,
    "  open connect to MPIO player",
    mpiosh_cmd_open, NULL },
  { "close", NULL,
    "  close connect to MPIO player",
    mpiosh_cmd_close, NULL },
  { "quit", " or exit",
    "exit mpiosh and close the device",
    mpiosh_cmd_quit, NULL },
  { "exit", NULL, NULL, mpiosh_cmd_quit },
  { "get", "<filename>",
    "read <filename> from memory card",
    mpiosh_cmd_get, mpiosh_readline_comp_mpio_file },
  { "mget", "<regexp>",
    "  read all files matching the regular expression\n"
    "  from the selected memory card",
    mpiosh_cmd_mget, mpiosh_readline_comp_mpio_file },
  { "put", "<filename>",
    "  write <filename> to memory card",
    mpiosh_cmd_put, mpiosh_readline_comp_mpio_file },
  { "mput", "<regexp>",
    "  write all local files matching the regular expression\n"
    "  to the selected memory card",
    mpiosh_cmd_mput, mpiosh_readline_comp_mpio_file },
  { "del", "<filename>",
    "  deletes <filename> from memory card",
    mpiosh_cmd_del, mpiosh_readline_comp_mpio_file },
  { "mdel", "<regexp>",
    "  deletes all files matching the regular expression\n"
    "  from the selected memory card",
    mpiosh_cmd_mdel, mpiosh_readline_comp_mpio_file },
  { "dump", NULL,
    "  get all files of current memory card",
    mpiosh_cmd_dump, NULL },
  { "free", NULL,
    "  display amount of available bytes of current memory card",
    mpiosh_cmd_free, NULL },
  { "format", NULL,
    "  format current memory card",
    mpiosh_cmd_format, NULL },
/*   { "switch", "<file1> <file2>", */
/*     "switches the order of two files", */
/*     mpiosh_cmd_switch }, */
  { "ldir", NULL,
    "  list local directory",
    mpiosh_cmd_ldir, NULL },
  { "lpwd", NULL,
    "  print current working directory",
    mpiosh_cmd_lpwd, NULL },
  { "lcd", NULL,
    "  change the current working directory",
    mpiosh_cmd_lcd, NULL },
  { "lmkdir", NULL,
    "  create a local directory",
    mpiosh_cmd_lmkdir, NULL },
  { "dump_memory", NULL,
    "  dump FAT, directory, spare area and the first 0x100 of the\n"
    "  selected memory card",
    mpiosh_cmd_dump_mem, NULL },
  { NULL, NULL, NULL, NULL, NULL }
};

/* mpiosh core functions */
void
mpiosh_init(void)
{
  mpiosh.dev = NULL;
  mpiosh.card = MPIO_INTERNAL_MEM;
  mpiosh.prompt = PROMPT_INT;
}

void
mpiosh_signal_handler(int signal)
{
  mpiosh_cancel = 1;
  mpiosh_cancel_ack = 0;
}

int
main(int argc, char *argv[]) {
  char *		line;
  char **		cmds, **walk;
  mpiosh_cmd_t *	cmd;
  struct sigaction	sigc;
  int			interactive = 1;
  
  UNUSED(argc);
  UNUSED(argv);
  
  setenv("mpio_debug", "", 0);
  setenv("mpio_color", "", 0);

  /* no unwanted interruption anymore */
  sigc.sa_handler = mpiosh_signal_handler;
  sigc.sa_flags = SA_NOMASK;
  
  sigaction(SIGINT, &sigc, NULL);
  
  /* init readline and history */
  mpiosh_readline_init();
  using_history();
  
  debug_init();
  
  mpiosh_init();
  mpiosh.dev = mpio_init(mpiosh_callback_init);
  printf("\n");

  if (mpiosh.card == MPIO_INTERNAL_MEM)
    mpiosh.prompt = PROMPT_INT;
  else
    mpiosh.prompt = PROMPT_EXT;

  if (!isatty(fileno(stdin))) {
    interactive = 0;
    mpiosh.prompt = NULL;
    mpiosh_readline_pipe();
  }
  
  if (!mpiosh.dev && interactive) {
    printf("could not find MPIO player.\n");
  }

  while ((line = readline(mpiosh.prompt))) {
    if ((*line == '\0') || mpiosh_cancel) {
      rl_clear_pending_input ();
      mpiosh_cancel = 0;
      mpiosh_cancel_ack = 0;
      continue;
    }

    cmds = mpiosh_command_split_line(line);

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
      
	if (!interactive) debug("running... '%s'\n", *walk);
	cmd->cmd_func(args);
	mpiosh_command_free_args(args);
      } else
	fprintf(stderr, "unknown command: '%s'\n", *walk);

/*       if ((idx = history_search(line, -1)) != -1) */
/* 	history_set_pos(idx); */
/*       else */
      add_history(line);
      free(*walk);
      walk++;
    }
    free(cmds);

    /* reset abort state */
    mpiosh_cancel = 0;    
  }

  mpiosh_cmd_quit(NULL);

  return 0;
}
