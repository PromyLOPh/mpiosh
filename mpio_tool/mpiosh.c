/* -*- linux-c -*- */

/* 
 *
 * $Id: mpiosh.c,v 1.3 2002/09/01 18:27:49 crunchy Exp $
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

#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "libmpio/debug.h"
#include "libmpio/mpio.h"

#include "mpiosh.h"

static mpiosh_t mpiosh;

static mpiosh_cmd_t commands[] = {
  { "debug", 	mpiosh_cmd_debug,	YES },
  { "ver", 	mpiosh_cmd_version,	NO },
  { "help", 	mpiosh_cmd_help,	NO },
  { "dir",	mpiosh_cmd_dir,		NO },
  { "info",	mpiosh_cmd_info,	NO },
  { "mem",	mpiosh_cmd_mem,		YES },
  { "open",	mpiosh_cmd_open,	NO },
  { "close",	mpiosh_cmd_close,	NO },
  { "quit",	mpiosh_cmd_quit,	NO },
  { "exit",	mpiosh_cmd_quit,	NO },
  { "get",	mpiosh_cmd_get,		YES },
  { "put",	mpiosh_cmd_put,		YES },
  { "del",	mpiosh_cmd_del,		YES },
  { "dump",	mpiosh_cmd_dump,	NO },
  { "free",	mpiosh_cmd_free,	NO },
  { "format",	mpiosh_cmd_format,	NO },
  { "switch",	mpiosh_cmd_switch,	YES },
  { "ldir",	mpiosh_cmd_ldir,	YES },
  { "lcd",	mpiosh_cmd_lcd,		YES },
  { "lmkdir",	mpiosh_cmd_lmkdir,	YES },
  { NULL, NULL, NO }
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

/* commands */
void
mpiosh_cmd_debug(char *args[])
{
  if (args[0] != NULL) {
    if (!strcmp(args[0], "level")) {
      debug_level(strtol(args[1], NULL, 0));
    } else if (!strcmp(args[0], "file")) {
      debug_file(args[1]);
    } else if (!strcmp(args[0], "on")) {
      if (debug_level_get() == -1)
	debug_level(1);
      else
	fprintf(stderr, "debug already activated for level %d\n",
		debug_level_get());
    } else if (!strcmp(args[0], "off")) {
      if (debug_level_get() == -1)
	fprintf(stderr,	"debug already deactivated\n");
      else
	debug_level(-1);
    } else {
      fprintf(stderr, "unknown debug command\n");
      printf("debug [level|file|on|off] <value>\n");
    }
  } else {
    fprintf(stderr, "error: no arguments given\n");
    printf("debug [level|file|on|off] <value>\n");
  }  
}

void
mpiosh_cmd_version(char *args[])
{
  UNUSED(args);
  
  printf("MPIO shell %s\n\n", VERSION);
}

void
mpiosh_cmd_help(char *args[])
{
  UNUSED(args);
  
  printf("debug[level|file|on|off] <value>\n"
	 "  modify debugging options\n");
  printf("ver\n"
	 "  version of mpio package\n");
  printf("help\n"
	 "  show known commands\n");
  printf("info\n"
	 "  show information about MPIO player\n");
  printf("open\n"
	 "  open connect to MPIO player\n");
  printf("close\n"
	 "  close connect to MPIO player\n");
  printf("mem [i|e]\n"
	 "  set current memory card. 'i' selects the internal and 'e'\n"
	 "  selects the external memory card (smart media card)\n");
  printf("dir\n"
	 "  list content of current memory card\n");
  printf("format\n"
	 "  format current memory card\n");
  printf("dump\n"
	 "  get all files of current memory card\n");
  printf("free\n"
	 "  display amount of available bytes of current memory card\n");
  printf("put <filename>\n"
	 "  write <filename> to memory card\n");
  printf("get <filename>\n"
	 "  read <filename> from memory card\n");
  printf("del <filename>\n"
	 "  deletes <filename> from memory card\n");
  printf("exit, quit\n"
	 "  exit mpiosh and close the device\n");
  printf("lcd\n"
	 "  change the current working directory\n");
  printf("ldir\n"
	 "  list local directory\n");
  printf("lmkdir\n"
	 "  create a local directory\n");
}

void
mpiosh_cmd_dir(char *args[])
{
  BYTE *p;
  BYTE month, day, hour, minute;
  BYTE fname[100];
  WORD year;  
  DWORD fsize;  
  
  UNUSED(args);
  
  if (mpiosh.dev == NULL) {
    printf("open connection first!\n");
    return;
  }
  
  p = mpio_directory_open(mpiosh.dev, mpiosh.card);
  while (p != NULL) {
    memset(fname, '\0', 100);
    
    mpio_dentry_get(mpiosh.dev, p,
		    fname, 100,
		    &year, &month, &day,
		    &hour, &minute, &fsize);

    printf ("%02d.%02d.%04d %02d:%02d  %9d - %s\n",
	    day, month, year, hour, minute, fsize, fname);

    p = mpio_dentry_next(mpiosh.dev, p);
  }  
}

void
mpiosh_cmd_info(char *args[])
{
  mpio_info_t info;
  
  UNUSED(args);
  
  if (mpiosh.dev == NULL) {
    printf("open connection first!\n");
    return;
  }
  
  mpio_get_info(mpiosh.dev, &info);
  
  printf("firmware %s\n", info.firmware_id);
  printf(" version : %s\n", info.firmware_version);
  printf(" date    : %s\n", info.firmware_date);
  printf("memory\n");
  printf(" internal: %s\n", info.firmware_mem_internal);
  printf(" external: %s\n", info.firmware_mem_external);
}

void
mpiosh_cmd_mem(char *args[])
{
  if (mpiosh.dev == NULL) {
    printf("open connection first!\n");
    return;
  }
  
  if (args[0] == NULL) {
    printf("error: no argument given.\n");
    return;
  }

  if (!strcmp(args[0], "e")) {
    mpiosh.card = MPIO_EXTERNAL_MEM;
    mpiosh.prompt = PROMPT_EXT;
    printf("external memory card is selected\n");
  } else if (!strcmp(args[0], "i")) {
    mpiosh.card = MPIO_INTERNAL_MEM;
    mpiosh.prompt = PROMPT_INT;
    printf("internal memory card is selected\n");
  } else {
    printf("can not select memory card '%s'\n", args[0]);
  }
}

void
mpiosh_cmd_open(char *args[])
{
  if (mpiosh.dev) {
    printf("connection to MPIO player already opened\n");
    return;
  }
  
  UNUSED(args);
  
  mpiosh.dev = mpio_init();

  if (mpiosh.dev == NULL)	
    printf("error: could not open connection MPIO player\n");
  else
    printf("connection to MPIO player is opened\n");
}

void
mpiosh_cmd_close(char *args[]) 
{
  if (mpiosh.dev == NULL) {
    printf("connection to MPIO player already closed\n");
    return;
  }
  
  UNUSED(args);
  
  mpio_close(mpiosh.dev);
  mpiosh.dev = NULL;

  printf("connection to MPIO player is closed\n");
}

void
mpiosh_cmd_quit(char *args[])
{
  if (mpiosh.dev) {
    printf("\nclosing connection to MPIO player\nHave a nice day\n");
    mpio_close(mpiosh.dev);
  }

  UNUSED(args);
  
  exit(0);
}

BYTE
mpiosh_callback_get(int read, int total) 
{
  printf("\rretrieved %.2f %%", ((double) read / total) * 100.0 );
  fflush(stdout);
  return 0; // continue
}

void
mpiosh_cmd_get(char *args[])
{
  int size;
  
  if (mpiosh.dev == NULL) {
    printf("connection to MPIO player already closed\n");
    return;
  }
  
  if (args[0] == NULL) {
    printf("error: no argument given\n");
    return;
  }
  
  size = mpio_file_get(mpiosh.dev, mpiosh.card, args[0], mpiosh_callback_get);

  printf("\n");
}

BYTE
mpiosh_callback_put(int read, int total) 
{
  printf("\rwrote %.2f %%", ((double) read / total) * 100.0 );
  fflush(stdout);
  return 0; // continue
}

void
mpiosh_cmd_put(char *args[])
{
  int size;
  
  if (mpiosh.dev == NULL) {
    printf("connection to MPIO player already closed\n");
    return;
  }
  
  if (args[0] == NULL) {
    printf("error: no argument given\n");
    return;
  }
  
  size = mpio_file_put(mpiosh.dev, mpiosh.card, args[0], mpiosh_callback_put);

  printf("\n");
}

BYTE
mpiosh_callback_del(int read, int total) 
{
  printf("\rdeleted %.2f %%", ((double) read / total) * 100.0 );
  fflush(stdout);
  return 0; // continue
}

void
mpiosh_cmd_del(char *args[])
{
  int size;
  
  if (mpiosh.dev == NULL) {
    printf("connection to MPIO player already closed\n");
    return;
  }
  
  if (args[0] == NULL) {
    printf("error: no argument given\n");
    return;
  }
  
  size = mpio_file_del(mpiosh.dev, mpiosh.card, args[0], mpiosh_callback_del);

  printf("\n");
}


void
mpiosh_cmd_dump(char *args[])
{
  BYTE *p;
  BYTE month, day, hour, minute;
  BYTE fname[256];
  char *arg[2];
  WORD year;  
  DWORD fsize;  
  
  if (mpiosh.dev == NULL) {
    printf("open connection first!\n");
    return;
  }
  
  UNUSED(args);
  
  arg[0] = fname;
  arg[1] = NULL;
  
  p = mpio_directory_open(mpiosh.dev, mpiosh.card);
  while (p != NULL) {
    memset(fname, '\0', 256);
    
    mpio_dentry_get(mpiosh.dev, p,
		    fname, 256,
		    &year, &month, &day,
		    &hour, &minute, &fsize);

    
    printf("getting '%s' ... \n", arg[0]);
    mpiosh_cmd_get(arg);
    
    p = mpio_dentry_next(mpiosh.dev, p);
  }  
}

void
mpiosh_cmd_free(char *args[])
{
  int free, kbytes;
  
  if (mpiosh.dev == NULL) {
    printf("open connection first!\n");
    return;
  }

  UNUSED(args);
  
  kbytes = mpio_memory_free(mpiosh.dev, mpiosh.card, &free);

  printf("%d KB of %d KB are available\n", free, kbytes);
}

BYTE
mpiosh_callback_format(int read, int total) 
{
  printf("\rformatted %.2f %%", ((double) read / total) * 100.0 );
  fflush(stdout);
  return 0; // continue
}

void
mpiosh_cmd_format(char *args[])
{
  char answer[512];
  
  if (mpiosh.dev == NULL) {
    printf("open connection first!\n");
    return;
  }

  UNUSED(args);
  
  printf("This will destroy all tracks saved on the memory card. "
	 "Are you sure (y/n)? ");

  fgets(answer, 511, stdin);
  
  if (answer[0] == 'y' || answer[0] == 'Y') {
    if (mpio_memory_format(mpiosh.dev, mpiosh.card,
			   mpiosh_callback_format) == -1)
      printf("\nfailed\n");
    else
      printf("\n");
  }
}

void
mpiosh_cmd_switch(char *args[])
{
  if (mpiosh.dev == NULL) {
    printf("open connection first!\n");
    return;
  }

  UNUSED(args);
}

void
mpiosh_cmd_ldir(char *args[])
{
  char			dir_buf[NAME_MAX];
  struct dirent **	dentry, **run;
  int			count;
  
  getcwd(dir_buf, NAME_MAX);
  if (dir_buf != '\0') {
    if ((count = scandir(dir_buf, &dentry, NULL, alphasort)) != -1) {
      int		j, i, len = 0;
      struct stat	st;
      struct passwd *	pwd;
      struct group *	grp;
      char 		time[12];
      char		rights[11];
      
      run = dentry;
      rights[10] = '\0';
      for (i = 0; i < count; i++, run++) {
	stat((*run)->d_name, &st);

	rights[0] = *("?pc?dnb?-?l?s???" + (st.st_mode >> 12 & 0xf));
	for (j = 0; j < 9; j++) {
	  if (st.st_mode & 1 << (8 - j))
            rights[j + 1] = "rwxrwxrwx"[j];
	  else
            rights[j + 1] = '-';
	}

	pwd = getpwuid(st.st_uid);
	grp = getgrgid(st.st_gid);
	strftime(time, 12, "%b %2d", localtime(&(st.st_mtime)));
	printf("%s %08s %08s %8d %10s %s\n",
	       rights, pwd->pw_name, grp->gr_name, st.st_size,
	       time,
	       (*run)->d_name);
	free(*run);
      }
      free(dentry);
    }    
  }
}

void
mpiosh_cmd_lcd(char *args[])
{
  if (args[0] == NULL) {
    fprintf(stderr, "error: no argument given\n");
    return;
  }
  
  if (chdir(args[0])) {
    perror ("error");
  } 
}

void
mpiosh_cmd_lmkdir(char *args[])
{
  if (args[0] == NULL) {
    fprintf(stderr, "error: no argument given\n");
    return;
  }

  if (mkdir(args[0], 0777)) {
    perror("error");
  }
}


int
main(int argc, char *argv[]) {
  char *	line;
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
    
    cmd = mpiosh_command_find(line);
    
    if (cmd) {
      char ** help, **args = mpiosh_command_get_args(line);
      help = args;
      
      cmd->func(args);
      mpiosh_command_free_args(args);
    } else
      printf("unknown command: %s\n", line);
    
    add_history(line);
  }

  mpiosh_cmd_quit(NULL);

  return 0;
}

  
