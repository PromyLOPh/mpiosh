/* callback.c
 *
 * Author: Andreas B�sching  <crunchy@tzi.de>
 *
 * $Id: callback.c,v 1.1 2002/09/13 06:59:38 crunchy Exp $
 *
 * Copyright (C) 2001 Andreas B�sching <crunchy@tzi.de>
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

#include "callback.h"

#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <regex.h>
#include <time.h>
#include <unistd.h>

#include "mpiosh.h"

#include "libmpio/debug.h"

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
  mpiosh_cmd_t *cmd = commands;

  UNUSED(args);
 
  while (cmd->cmd) {
    printf("%s", cmd->cmd);
    if (cmd->args)
      printf(" %s\n", cmd->args);
    else
      printf("\n");
    if (cmd->info)
      printf("  %s\n", cmd->info);
    else
      printf("\n");
    cmd++;
  }
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
  
  MPIOSH_CHECK_CONNECTION_CLOSED;
  
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
  
  MPIOSH_CHECK_CONNECTION_CLOSED;
  
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
  MPIOSH_CHECK_CONNECTION_CLOSED;
  MPIOSH_CHECK_ARG;

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
  MPIOSH_CHECK_CONNECTION_OPEN;
  
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
  MPIOSH_CHECK_CONNECTION_CLOSED;
  
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
  
  MPIOSH_CHECK_CONNECTION_CLOSED;
  MPIOSH_CHECK_ARG;
  
  size = mpio_file_get(mpiosh.dev, mpiosh.card, args[0], mpiosh_callback_get);

  printf("\n");
}

void
mpiosh_cmd_mget(char *args[])
{
  BYTE *	p;
  int		size, i = 0;
  int           error;
  regex_t	regex;
  BYTE		fname[100];
  BYTE          errortext[100];
  BYTE		month, day, hour, minute;
  WORD		year;  
  DWORD		fsize;  

  MPIOSH_CHECK_CONNECTION_CLOSED;
  MPIOSH_CHECK_ARG;
  
  while (args[i] != NULL) {
    if ((error = regcomp(&regex, args[i], REG_NOSUB))) {
      regerror(error, &regex, errortext, 100);
      debugn (2, "error in regular expression: %s (%s)\n", args[i], errortext);
    } else {
      p = mpio_directory_open(mpiosh.dev, mpiosh.card);
      while (p != NULL) {
	memset(fname, '\0', 100);
	mpio_dentry_get(mpiosh.dev, p, fname, 100,
			&year, &month, &day, &hour, &minute, &fsize);
	
	if (!(error = regexec(&regex, fname, 0, NULL, 0))) {
	  printf("getting '%s' ... \n", fname);
	  size = mpio_file_get(mpiosh.dev, mpiosh.card,
			       fname, mpiosh_callback_put);
	  printf("\n");
	} else {
	  regerror(error, &regex, errortext, 100);
	  debugn (2, "file does not match: %s (%s)\n", fname, errortext);
	}
	
	p = mpio_dentry_next(mpiosh.dev, p);
      }
    }
    i++;
  }
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
  
  MPIOSH_CHECK_CONNECTION_CLOSED;
  MPIOSH_CHECK_ARG;
  
  size = mpio_file_put(mpiosh.dev, mpiosh.card, args[0], mpiosh_callback_put);

  printf("\n");
}

void
mpiosh_cmd_mput(char *args[])
{
  char			dir_buf[NAME_MAX];
  int			size, j, i = 0;
  struct dirent **	dentry, **run;
  regex_t	        regex;
  int                   error;
  BYTE                  errortext[100];
  int                   fsize;

  MPIOSH_CHECK_CONNECTION_CLOSED;
  MPIOSH_CHECK_ARG;
  
  getcwd(dir_buf, NAME_MAX);
  while (args[i] != NULL) {
    if ((error = regcomp(&regex, args[i], REG_NOSUB))) {
      regerror(error, &regex, errortext, 100);
      debugn (2, "error in regular expression: %s (%s)\n", args[i], errortext);
    } else {
      if ((size = scandir(dir_buf, &dentry, NULL, alphasort)) != -1) {
	run = dentry;
	for (j = 0; j < size; j++, run++) {
	  if (!(error = regexec(&regex, (*run)->d_name, 0, NULL, 0))) {
	    printf("putting '%s' ... \n", (*run)->d_name);
	    fsize = mpio_file_put(mpiosh.dev, mpiosh.card,
				 (*run)->d_name, mpiosh_callback_put);
	    printf("\n");
	  } else {
	    regerror(error, &regex, errortext, 100);
	    debugn (2, "file does not match: %s (%s)\n", 
		    (*run)->d_name, errortext);
	  }
	  free(*run);
	}
	free(dentry);
      }
    }
    i++;
  }
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
  
  MPIOSH_CHECK_CONNECTION_CLOSED;
  MPIOSH_CHECK_ARG;
  
  size = mpio_file_del(mpiosh.dev, mpiosh.card, args[0], mpiosh_callback_del);

  printf("\n");
}

void
mpiosh_cmd_mdel(char *args[])
{
  BYTE *	p;
  int		size, i = 0;
  int           error;
  regex_t	regex;
  BYTE		fname[100];
  BYTE          errortext[100];
  BYTE		month, day, hour, minute;
  WORD		year;  
  DWORD		fsize;  

  MPIOSH_CHECK_CONNECTION_CLOSED;
  MPIOSH_CHECK_ARG;
  
  while (args[i] != NULL) {
    if ((error = regcomp(&regex, args[i], REG_NOSUB))) {
      regerror(error, &regex, errortext, 100);
      debugn (2, "error in regular expression: %s (%s)\n", args[i], errortext);
    } else {
      p = mpio_directory_open(mpiosh.dev, mpiosh.card);
      while (p != NULL) {
	memset(fname, '\0', 100);
	mpio_dentry_get(mpiosh.dev, p, fname, 100,
			&year, &month, &day, &hour, &minute, &fsize);
	
	if (!(error = regexec(&regex, fname, 0, NULL, 0))) {
	  printf("deleting '%s' ... \n", fname);
	  size = mpio_file_del(mpiosh.dev, mpiosh.card,
			       fname, mpiosh_callback_del);
	  printf("\n");
	  /* if we delete a file, start again from the beginning, 
	     because the directory has changed !! */
	  p = mpio_directory_open(mpiosh.dev, mpiosh.card);
	} else {
	  regerror(error, &regex, errortext, 100);
	  debugn (2, "file does not match: %s (%s)\n", fname, errortext);
	  p = mpio_dentry_next(mpiosh.dev, p);
	}
	
      }
    }
    i++;
  }
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
  
  MPIOSH_CHECK_CONNECTION_CLOSED;
  
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
  
  MPIOSH_CHECK_CONNECTION_CLOSED;

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
  
  MPIOSH_CHECK_CONNECTION_CLOSED;

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
  MPIOSH_CHECK_CONNECTION_CLOSED;

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
      int		j, i;
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
	printf("%s %8s %8s %8d %10s %s\n",
	       rights, pwd->pw_name, grp->gr_name, (int)st.st_size,
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
  MPIOSH_CHECK_ARG;
  
  if (chdir(args[0])) {
    perror ("error");
  } 
}

void
mpiosh_cmd_lmkdir(char *args[])
{
  MPIOSH_CHECK_ARG;

  if (mkdir(args[0], 0777)) {
    perror("error");
  }
}

/* end of callback.c */
