/* -*- linux-c -*- */

/* 
 *
 * $Id: mpiosh.c,v 1.21 2002/10/29 20:03:35 crunchy Exp $
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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "libmpio/debug.h"
#include "libmpio/mpio.h"

#include "callback.h"
#include "command.h"
#include "config.h"
#include "readline.h"
#include "mpiosh.h"

/* mpiosh core functions */
void
mpiosh_init(void)
{
  /* set state */
  mpiosh.dev = NULL;
  mpiosh.config = NULL;
  mpiosh.prompt = NULL;

  /* read configuration */
  mpiosh.config = mpiosh_config_new();
  if (mpiosh.config)
    mpiosh_config_read(mpiosh.config);

  mpiosh.card = mpiosh.config->default_mem;
  if (mpiosh.config->default_mem == MPIO_EXTERNAL_MEM)
    mpiosh.prompt = mpiosh.config->prompt_ext;
  else
    mpiosh.prompt = mpiosh.config->prompt_int;

  /* inital mpio library */
  mpiosh.dev = mpio_init(mpiosh_callback_init);

  printf("\n");
}

void
mpiosh_signal_handler(int signal)
{
  mpiosh_cancel = 1;
  mpiosh_cancel_ack = 0;
}

int
main(int argc, char *argv[]) {
  char			*line;
  char			**cmds, **walk;
  struct mpiosh_cmd_t 	*cmd;
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
