/* callback.c
 *
 * Author: Andreas Büsching  <crunchy@tzi.de>
 *
 * $Id: callback.c,v 1.43 2003/06/27 12:21:21 crunchy Exp $
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

#include "callback.h"

#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <regex.h>
#include <time.h>
#include <unistd.h>

#include "mpiosh.h"
#include "command.h"

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
  struct mpiosh_cmd_t	*cmd = commands;
  int			ignore;

  while (cmd->cmd) {
    if (args[0] != NULL) {
      char **	walk = args;
      ignore = 1;
      
      while(*walk) {
	if (!strcmp(*walk, cmd->cmd)) {
	  ignore = 0;
	  break;
	}
	walk++;
      }
    } else
      ignore = 0;
    
    if (!ignore) {
      printf("%s", cmd->cmd);
      if (cmd->args)
	printf(" %s\n", cmd->args);
      else
	printf("\n");
      if (cmd->aliases) {
	char **go = cmd->aliases;
	printf(" alias:\n  ");
	while(*go) printf(( *(go+1) ? "%s" : "%s, "), *go++);
	printf("\n");
      }
      
      if (cmd->info)
	printf(" description:\n%s\n", cmd->info);
      else
	printf("\n");
    }
    cmd++;
  }
}

void
mpiosh_cmd_dir(char *args[])
{
  BYTE *p;
  BYTE month, day, hour, minute, type;
  BYTE fname[100];
  WORD year;  
  DWORD fsize;  
  
  UNUSED(args);
  
  MPIOSH_CHECK_CONNECTION_CLOSED;
  
  p = mpio_directory_open(mpiosh.dev, mpiosh.card);
  while (p != NULL) {
    memset(fname, '\0', 100);
    
    mpio_dentry_get(mpiosh.dev, mpiosh.card, p,
		    fname, 100,
		    &year, &month, &day,
		    &hour, &minute, &fsize,
		    &type);

    printf ("%02d.%02d.%04d %02d:%02d  %9d %c %s\n",
	    day, month, year, hour, minute, fsize, type, fname);

    p = mpio_dentry_next(mpiosh.dev, mpiosh.card, p);
  }  
}

void
mpiosh_cmd_pwd(char *args[])
{
  BYTE pwd[INFO_LINE];
  
  UNUSED(args);
  
  MPIOSH_CHECK_CONNECTION_CLOSED;

  mpio_directory_pwd(mpiosh.dev, mpiosh.card, pwd);
  printf ("%s\n", pwd);
  
}

void
mpiosh_cmd_mkdir(char *args[])
{
  BYTE pwd[INFO_LINE];
  
  MPIOSH_CHECK_CONNECTION_CLOSED;

  mpio_directory_make(mpiosh.dev, mpiosh.card, args[0]);
  mpio_sync(mpiosh.dev, mpiosh.card);

}

void
mpiosh_cmd_cd(char *args[])
{
  BYTE pwd[INFO_LINE];
  
  MPIOSH_CHECK_CONNECTION_CLOSED;

  mpio_directory_cd(mpiosh.dev, mpiosh.card, args[0]);

  mpio_directory_pwd(mpiosh.dev, mpiosh.card, pwd);
  printf ("directory is now: %s\n", pwd);
  
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
  printf("model    : %s\n", info.model);
  printf("memory\n");
  printf(" internal: %s\n", info.mem_internal);
  printf(" external: %s\n", info.mem_external);
}

void
mpiosh_cmd_mem(char *args[])
{
  int free;
  
  MPIOSH_CHECK_CONNECTION_CLOSED;
  MPIOSH_CHECK_ARG;

  if (!strcmp(args[0], "e")) {
    if (mpio_memory_free(mpiosh.dev, MPIO_EXTERNAL_MEM, &free)) {
      mpiosh.card = MPIO_EXTERNAL_MEM;
      mpiosh.prompt = PROMPT_EXT;
      printf("external memory card is selected\n");
    } else {
      printf("no external memory card is available\n");
    }
    
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
  
  mpiosh.dev = mpio_init(mpiosh_callback_init);

  printf("\n");

  if (mpiosh.dev == NULL) {
    mpio_perror("ERROR");
    printf("could not open connection MPIO player\n");
  }
  else
    printf("connection to MPIO player is opened\n");

  if ((mpiosh.dev) && (mpiosh.config->charset))
    mpio_charset_set(mpiosh.dev, mpiosh.config->charset);
  if (mpiosh.dev) {
    mpio_id3_set(mpiosh.dev, mpiosh.config->id3_rewriting);
    mpio_id3_format_set(mpiosh.dev, mpiosh.config->id3_format);
  } 
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
  
  if (mpiosh.config) {
    mpiosh_config_write(mpiosh.config);
    mpiosh_config_free(mpiosh.config);
  }

  exit(0);
}

BYTE
mpiosh_callback_init(mpio_mem_t mem, int read, int total) 
{
  switch(mem) 
    {
    case MPIO_INTERNAL_MEM:
      printf("\rinternal memory: " );
      break;
    case MPIO_EXTERNAL_MEM:
      printf("\rexternal memory: " );
      break;
    default:
      printf("\runknown  memory: " );
    }
  
  printf("initialized %.2f %% ", ((double) read / total) * 100.0 );
  fflush(stdout);

  return mpiosh_cancel; // continue
}

BYTE
mpiosh_callback_get(int read, int total) 
{
  printf("\rretrieved %.2f %%", ((double) read / total) * 100.0 );
  fflush(stdout);

  if (mpiosh_cancel) 
    debug ("user cancelled operation\n");
  
  return mpiosh_cancel; // continue
}

/* void */
/* mpiosh_cmd_get(char *args[]) */
/* { */
/*   MPIOSH_CHECK_CONNECTION_CLOSED; */
/*   MPIOSH_CHECK_ARG; */
  
/*   if (mpio_file_get(mpiosh.dev, mpiosh.card, args[0],  */
/* 		    mpiosh_callback_get) == -1) { */
/*     mpio_perror("error"); */
/*   }  */
/*   printf("\n"); */
/* } */

void
mpiosh_cmd_mget(char *args[])
{
  BYTE *	p;
  int		i = 0, error;
  regex_t	regex;
  BYTE		fname[100];
  BYTE          errortext[100];
  BYTE		month, day, hour, minute, type;
  WORD		year;  
  DWORD		fsize;  
  int           found;

  MPIOSH_CHECK_CONNECTION_CLOSED;
  MPIOSH_CHECK_ARG;

  mpiosh_command_regex_fix(args);
  
  while (args[i] != NULL) {
    if ((error = regcomp(&regex, args[i], REG_NOSUB))) {
      regerror(error, &regex, errortext, 100);
      debugn (2, "error in regular expression: %s (%s)\n", args[i], errortext);
    } else {
      found = 0;
      p = mpio_directory_open(mpiosh.dev, mpiosh.card);
      while (p != NULL) {
	memset(fname, '\0', 100);
	mpio_dentry_get(mpiosh.dev, mpiosh.card, p, fname, 100,
			&year, &month, &day, &hour, &minute, &fsize, &type);
	
	if (!(error = regexec(&regex, fname, 0, NULL, 0))) {
	  found = 1;
	  printf("getting '%s' ... \n", fname);
	  if ((mpio_file_get(mpiosh.dev, mpiosh.card,
				    fname, mpiosh_callback_get)) == -1) {
	    debug("cancelled operation\n");
	    mpio_perror("error");
	    break;
	  } 
	  printf("\n");
	  if (mpiosh_cancel) {
	    debug("operation cancelled by user\n");
	    break;
	  }
	} else {
	  regerror(error, &regex, errortext, 100);
	  debugn (2, "file does not match: %s (%s)\n", fname, errortext);
	}
	
	p = mpio_dentry_next(mpiosh.dev, mpiosh.card, p);
      }
    }
    if (!found)
      printf("file not found! (%s)\n", args[i]);
    i++;
  }


  regfree(&regex);
}

BYTE
mpiosh_callback_put(int read, int total) 
{
  printf("\rwrote %.2f %%", ((double) read / total) * 100.0 );
  fflush(stdout);

  if ((mpiosh_cancel) && (!mpiosh_cancel_ack)) {
    debug ("user cancelled operation\n");
    mpiosh_cancel_ack = 1;
  }
  
  return mpiosh_cancel; // continue
}

/* void */
/* mpiosh_cmd_put(char *args[]) */
/* { */
/*   int size; */

/*   MPIOSH_CHECK_CONNECTION_CLOSED; */
/*   MPIOSH_CHECK_ARG; */
  
/*   if ((size = mpio_file_put(mpiosh.dev, mpiosh.card, args[0], FTYPE_MUSIC, */
/* 			    mpiosh_callback_put)) == -1) { */
/*     mpio_perror("error"); */
/*   } else { */
/*     mpio_sync(mpiosh.dev, mpiosh.card); */
/*   } */
/*   printf("\n"); */
/* } */

void
mpiosh_cmd_mput(char *args[])
{
  char			dir_buf[NAME_MAX];
  int			size, j, i = 0, error, written = 0;
  struct dirent **	dentry, **run;
  struct stat		st;
  regex_t	        regex;
  BYTE                  errortext[100];

  MPIOSH_CHECK_CONNECTION_CLOSED;
  MPIOSH_CHECK_ARG;
  
  mpiosh_command_regex_fix(args);
  getcwd(dir_buf, NAME_MAX);
  while (args[i] != NULL) {
    if ((error = regcomp(&regex, args[i], REG_NOSUB))) {
      regerror(error, &regex, errortext, 100);
      debugn (2, "error in regular expression: %s (%s)\n", args[i], errortext);
    } else {
      if ((size = scandir(dir_buf, &dentry, NULL, alphasort)) != -1) {
	run = dentry;
	for (j = 0; ((j < size) && (!mpiosh_cancel)); j++, run++) {
	  if (stat((*run)->d_name, &st) == -1) {
	    free(*run);
	    continue;
	  } else {
	    if (!S_ISREG(st.st_mode)) {
	      debugn(2, "not a regular file: '%s'\n", (*run)->d_name);
	      free(*run);
	      continue;
	    }
	  }
	  
	  if (!(error = regexec(&regex, (*run)->d_name, 0, NULL, 0))) {
	    printf("putting '%s' ... \n", (*run)->d_name);
	    if (mpio_file_put(mpiosh.dev, mpiosh.card, (*run)->d_name, 
			      FTYPE_MUSIC, mpiosh_callback_put) == -1) {
	      mpio_perror("error");
	      /* an existing file is no reason for a complete abort!! */
	      if (mpio_errno() == MPIO_ERR_FILE_EXISTS)
		continue;
	      break;
	    } 
	    written = 1; /* we did write something, so do mpio_sync afterwards */
	    
	    printf("\n");
	  } else {
	    regerror(error, &regex, errortext, 100);
	    debugn(2, "file does not match: %s (%s)\n", 
		   (*run)->d_name, errortext);
	  }
	  free(*run);
	}
	free(dentry);
      }
    }
    i++;
  }
  regfree(&regex);
  if (mpiosh_cancel) 
    debug("operation cancelled by user\n");

  if (written) {
    mpio_sync(mpiosh.dev, mpiosh.card);
  } else {
    printf("file not found!\n");    
  }
}

BYTE
mpiosh_callback_del(int read, int total) 
{
  printf("\rdeleted %.2f %%", ((double) read / total) * 100.0 );
  fflush(stdout);

  if (mpiosh_cancel) 
    debug ("user cancelled operation\n");

  return mpiosh_cancel; // continue
}

/* void */
/* mpiosh_cmd_del(char *args[]) */
/* { */
/*   int size; */
  
/*   MPIOSH_CHECK_CONNECTION_CLOSED; */
/*   MPIOSH_CHECK_ARG; */
  
/*   size = mpio_file_del(mpiosh.dev, mpiosh.card, args[0], mpiosh_callback_del); */
/*   mpio_sync(mpiosh.dev, mpiosh.card); */

/*   printf("\n"); */
/* } */

void
mpiosh_cmd_mdel(char *args[])
{
  BYTE *	p;
  int		i = 0;
  int           error;
  regex_t	regex;
  int           r;
  BYTE		fname[100];
  BYTE          errortext[100];
  BYTE		month, day, hour, minute, type;
  WORD		year;  
  DWORD		fsize;  
  int           deleted = 0;

  MPIOSH_CHECK_CONNECTION_CLOSED;
  MPIOSH_CHECK_ARG;
  
  mpiosh_command_regex_fix(args);
  while (args[i] != NULL) {
    if ((error = regcomp(&regex, args[i], REG_NOSUB))) {
      regerror(error, &regex, errortext, 100);
      debugn (2, "error in regular expression: %s (%s)\n", args[i], errortext);
    } else {
      p = mpio_directory_open(mpiosh.dev, mpiosh.card);
      while ((p != NULL) && (!mpiosh_cancel)) {
	memset(fname, '\0', 100);
	mpio_dentry_get(mpiosh.dev, mpiosh.card, p, fname, 100,
			&year, &month, &day, &hour, &minute, &fsize, &type);
	
	if ((!(error = regexec(&regex, fname, 0, NULL, 0))) &&
	    (strcmp(fname, "..")) && (strcmp(fname, ".")))
	  {
	  /* this line has to be above the del, or we won't write
	   * the FAT and directory in case of an abort!!!
	   */
	  deleted=1;
	  printf("deleting '%s' ... \n", fname);
	  r = mpio_file_del(mpiosh.dev, mpiosh.card,
			       fname, mpiosh_callback_del);
	  printf("\n");
	  if (mpiosh_cancel) break;
	  /* if we delete a file, start again from the beginning, 
	     because the directory has changed !! */
	  if (r != MPIO_OK)
	    {
	      mpio_perror("ERROR");
	      p = mpio_dentry_next(mpiosh.dev, mpiosh.card, p);
	      break;
	    }
	    p = mpio_directory_open(mpiosh.dev, mpiosh.card);
	} else {
	  regerror(error, &regex, errortext, 100);
	  debugn (2, "file does not match: %s (%s)\n", fname, errortext);
	  p = mpio_dentry_next(mpiosh.dev, mpiosh.card, p);
	}
	
      }
    }
    i++;
  }
  regfree(&regex);
  if (deleted) {
    mpio_sync(mpiosh.dev, mpiosh.card);
  } else {
    printf("file not found!\n");
  }
}


void
mpiosh_cmd_dump(char *args[])
{
  BYTE *p;
  BYTE month, day, hour, minute, type;
  BYTE fname[256];
  char *arg[2];
  WORD year;  
  DWORD fsize;  
  
  MPIOSH_CHECK_CONNECTION_CLOSED;
  
  UNUSED(args);
  
  arg[1] = NULL;
  
  p = mpio_directory_open(mpiosh.dev, mpiosh.card);
  while ((p != NULL) && (!mpiosh_cancel)) {
    arg[0] = fname; /* is this a memory leak?? -mager */
    memset(fname, '\0', 256);
    
    mpio_dentry_get(mpiosh.dev, mpiosh.card, p,
		    fname, 256,
		    &year, &month, &day,
		    &hour, &minute, &fsize, &type);

    mpiosh_cmd_mget(arg);
    
    p = mpio_dentry_next(mpiosh.dev, mpiosh.card, p);
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
  BYTE *config, *fmconfig, *rconfig, *fontconfig;
  int  csize, fmsize, rsize, fontsize;
  
  MPIOSH_CHECK_CONNECTION_CLOSED;

  UNUSED(args);

  printf("This will destroy all tracks saved on the memory card. "
	 "Are you sure (y/n)? ");

  fgets(answer, 511, stdin);
  
  if (answer[0] == 'y' || answer[0] == 'Y') {
    if (mpiosh.card == MPIO_INTERNAL_MEM) {
      printf("read config files from player\n");

      /* save config files and write them back after formatting */      
      config     = NULL;
      fmconfig   = NULL;
      rconfig    = NULL;
      fontconfig = NULL;

      csize = mpio_file_get_to_memory(mpiosh.dev, MPIO_INTERNAL_MEM, 
				      MPIO_CONFIG_FILE, NULL, &config);      
      fmsize = mpio_file_get_to_memory(mpiosh.dev, MPIO_INTERNAL_MEM, 
				       MPIO_CHANNEL_FILE, NULL, &fmconfig);
      rsize = mpio_file_get_to_memory(mpiosh.dev, MPIO_INTERNAL_MEM, 
				       MPIO_MPIO_RECORD, NULL, &rconfig);
      fontsize = mpio_file_get_to_memory(mpiosh.dev, MPIO_INTERNAL_MEM, 
				       MPIO_FONT_FON, NULL, &fontconfig);
    }

    printf("formatting memory...\n");

    if (mpio_memory_format(mpiosh.dev, mpiosh.card,
			   mpiosh_callback_format) == -1)
      printf("\nfailed\n");
    else {
      printf("\n");
      
      if (mpiosh.card == MPIO_INTERNAL_MEM) {
	printf("restoring saved config files\n");
	/* restore everything we saved */
	if (config)
	  if (mpio_file_put_from_memory(mpiosh.dev, MPIO_INTERNAL_MEM, 
				      MPIO_CONFIG_FILE, FTYPE_CONF,
				      NULL, config, csize)==-1)
	    mpio_perror("error");
	if (fmconfig)
	  if (mpio_file_put_from_memory(mpiosh.dev, MPIO_INTERNAL_MEM, 
					MPIO_CHANNEL_FILE, FTYPE_CHAN,
					NULL, fmconfig, fmsize)==-1)
	    mpio_perror("error");
	if (rconfig)
	  if (mpio_directory_make(mpiosh.dev, MPIO_INTERNAL_MEM, 
				  MPIO_MPIO_RECORD)!=MPIO_OK)
	    mpio_perror("error");
	if (fontconfig)
	  if (mpio_file_put_from_memory(mpiosh.dev, MPIO_INTERNAL_MEM, 
					MPIO_FONT_FON, FTYPE_FONT,
					NULL, fontconfig, fontsize)==-1)
	    mpio_perror("error");

	if (config || fmconfig || rconfig || fontconfig)
	  mpio_sync(mpiosh.dev, MPIO_INTERNAL_MEM);
	if (config)
	  free(config);
	if (fmconfig)
	  free(fmconfig);
	if (rconfig)
	  free(rconfig);
	if (fontconfig)
	  free(fontconfig);
      }
      
    } 
    mpiosh_cmd_health(NULL);
  }
}

void
mpiosh_cmd_switch(char *args[])
{
  MPIOSH_CHECK_CONNECTION_CLOSED;

  if(args[0] && args[1] && !args[2]) {
    if ((mpio_file_switch(mpiosh.dev, mpiosh.card,
			  args[0], args[1])) == -1) {
      mpio_perror("error");
    } else {
      mpio_sync(mpiosh.dev, mpiosh.card);
    }
    
  } else {
    fprintf(stderr, "error: wrong number of arguments given\n");
    printf("switch <file1> <file2>\n");
  }

}

void
mpiosh_cmd_rename(char *args[])
{
  MPIOSH_CHECK_CONNECTION_CLOSED;

  if(args[0] && args[1] && !args[2]) {
    if ((mpio_file_rename(mpiosh.dev, mpiosh.card,
			  args[0], args[1])) == -1) {
      mpio_perror("error");
    } else {
      mpio_sync(mpiosh.dev, mpiosh.card);
    }
    
  } else {
    fprintf(stderr, "error: wrong number of arguments given\n");
    printf("rename <oldfilename> <newfilename>\n");
  }
}

void
mpiosh_cmd_dump_mem(char *args[])
{
  
  MPIOSH_CHECK_CONNECTION_CLOSED;

  UNUSED(args);
  
  mpio_memory_dump(mpiosh.dev, mpiosh.card);

}

void
mpiosh_cmd_health(char *args[])
{
  mpio_health_t health;
  int i, lost;
  
  UNUSED(args);
  
  MPIOSH_CHECK_CONNECTION_CLOSED;
  
  mpio_health(mpiosh.dev, mpiosh.card, &health);
  
  if (mpiosh.card == MPIO_INTERNAL_MEM) {
    lost=0;
    printf("health status of internal memory:\n");
    printf("=================================\n");
    printf("%d chip%c      (total/spare/broken)\n",	   
	   health.num, ((health.num==1)?' ':'s'));
    for(i=0; i<health.num; i++) {
      printf("chip #%d      (%5d/%5d/%6d)\n", (i+1),
	     health.data[i].total,
	     health.data[i].spare,
	     health.data[i].broken);
      lost+=health.data[i].broken;      
    }    
    if (lost)
      printf("You have lost %d KB due to bad blocks.\n", lost*16);
  } 

  if (mpiosh.card == MPIO_EXTERNAL_MEM) {
    lost=0;
    printf("health status of external memory:\n");
    printf("=================================\n");
    printf("%d zone%c      (total/spare/broken)\n",	   
	   health.num, ((health.num==1)?' ':'s'));
    for(i=0; i<health.num; i++) {
      printf("zone #%d      (%5d/%5d/%6d)\n", (i+1),
	     health.data[i].total,
	     health.data[i].spare,
	     health.data[i].broken);
      if (health.data[i].spare<health.data[i].broken)
	lost++;
    }    
    if (lost)
      printf("%d zone%s to many broken blocks, expect trouble! :-(\n", lost, 
	     ((lost==1)?" has":"s have"));
  } 

  
}

void
mpiosh_cmd_backup(char *args[])
{
  int size;
  char filename[ 1024 ];
  
  UNUSED(args);
  
  MPIOSH_CHECK_CONNECTION_CLOSED;

  if ( !mpiosh_config_check_backup_dir( mpiosh.config, TRUE ) ) {
    fprintf( stderr, "error: could not create backup directory: %s\n",
	     CONFIG_BACKUP );
    return;
  }
  
  snprintf( filename, 1024, "%s%s", CONFIG_BACKUP, MPIO_CONFIG_FILE );
  size = mpio_file_get_as( mpiosh.dev, MPIO_INTERNAL_MEM,
			   MPIO_CONFIG_FILE, 
			   filename,
			   mpiosh_callback_get );
  if ( size == -1 ) return;
  if ( !size )
    debugn (1, "file does not exist: %s\n", MPIO_CONFIG_FILE );
    
  snprintf( filename, 1024, "%s%s", CONFIG_BACKUP, MPIO_CHANNEL_FILE );
  size = mpio_file_get_as( mpiosh.dev, MPIO_INTERNAL_MEM, 
			   MPIO_CHANNEL_FILE,
			   filename,
			   mpiosh_callback_get );
  if ( size == -1 ) return;
  if ( !size )
    debugn (2, "file does not exist: %s\n", MPIO_CHANNEL_FILE );

  snprintf( filename, 1024, "%s%s", CONFIG_BACKUP, MPIO_FONT_FON );
  size = mpio_file_get_as( mpiosh.dev, MPIO_INTERNAL_MEM, 
			   MPIO_FONT_FON,
			   filename,
			   mpiosh_callback_get );
  if ( size == -1 ) return;
  if ( !size )
    debugn (2, "file does not exist: %s\n", MPIO_FONT_FON );
}

void
mpiosh_cmd_restore(char *args[])
{
  int size;
  char filename[ 1024 ];

  UNUSED(args);
  
  MPIOSH_CHECK_CONNECTION_CLOSED;

  if ( !mpiosh_config_check_backup_dir( mpiosh.config, FALSE ) ) {
    fprintf( stderr, "error: there is no backup: %s\n",
	     CONFIG_BACKUP );
    return;
  }
  
  snprintf( filename, 1024, "%s%s", CONFIG_BACKUP, MPIO_CONFIG_FILE );
  size = mpio_file_put_as( mpiosh.dev, MPIO_INTERNAL_MEM,
			   filename,
			   MPIO_CONFIG_FILE, 
			   FTYPE_CONF, mpiosh_callback_put );
  if ( size == -1 ) return;
  if ( !size )
    debugn (1, "file does not exist: %s\n", MPIO_CONFIG_FILE );
    
  snprintf( filename, 1024, "%s%s", CONFIG_BACKUP, MPIO_CHANNEL_FILE );
  size = mpio_file_put_as( mpiosh.dev, MPIO_INTERNAL_MEM, 
			   filename,
			   MPIO_CHANNEL_FILE,
			   FTYPE_CHAN, mpiosh_callback_put );
  if ( size == -1 ) return;
  if ( !size )
    debugn (2, "file does not exist: %s\n", MPIO_CHANNEL_FILE );

  snprintf( filename, 1024, "%s%s", CONFIG_BACKUP, MPIO_FONT_FON );
  size = mpio_file_put_as( mpiosh.dev, MPIO_INTERNAL_MEM, 
			   filename,
			   MPIO_FONT_FON,
			   FTYPE_FONT, mpiosh_callback_put );
  if ( size == -1 ) return;
  if ( !size )
    debugn (2, "file does not exist: %s\n", MPIO_FONT_FON );
}


#if 0
void
mpiosh_cmd_config(char *args[])
{
  BYTE *config_data;
  int  size;

  MPIOSH_CHECK_CONNECTION_CLOSED;

  if (args[0] != NULL) {
    if (!strcmp(args[0], "read")) {
      if ((size = mpio_file_get(mpiosh.dev, MPIO_INTERNAL_MEM, 
				MPIO_CONFIG_FILE, NULL))<=0) {
	fprintf(stderr, "Could not read config file\n");
      } else {
	printf("done.\n");
      }
    } else if (!strcmp(args[0], "write")) {
      printf("deleting old config file ...\n");
      mpio_file_del(mpiosh.dev, MPIO_INTERNAL_MEM, 
		    MPIO_CONFIG_FILE, NULL);
      printf("writing new config file ...\n");
      if (mpio_file_put(mpiosh.dev, MPIO_INTERNAL_MEM, MPIO_CONFIG_FILE,
			FTYPE_CONF, NULL)==-1) 
	mpio_perror("error");
      mpio_sync(mpiosh.dev, MPIO_INTERNAL_MEM);
    } else if (!strcmp(args[0], "show")) {
      if ((size = mpio_file_get_to_memory(mpiosh.dev, MPIO_INTERNAL_MEM, 
				MPIO_CONFIG_FILE, NULL, &config_data))<=0) {
	fprintf(stderr, "Could not read config file\n");
      } else {
	hexdump(config_data, size);

	printf("current config:\n"
	       "===============\n");
	
	printf("volume:\t\t%-2d\n", config_data[0x00]);
	printf("repeat:\t\t");
	switch(config_data[0x01]) 
	  {
	  case 0:
	    printf("normal\n");
	    break;
	  case 1:
	    printf("repeat one\n");
	    break;
	  case 2:
	    printf("repeat all\n");
	    break;
	  case 3:
	    printf("shuffel\n");
	    break;
	  case 4:
	    printf("intro\n");
	    break;
	  default:
	    printf("unknown\n" );
	  }	      


	fprintf(stderr, "\nfinish my implementation please!\n");
	fprintf(stderr, "config values seem to be model dependant :-(\n");

	free(config_data);
      }
    } else {
      fprintf(stderr, "unknown config command\n");
      printf("config [read|write|show] channel\n");
    }
  } else {
    fprintf(stderr, "error: no arguments given\n");
    printf("config [read|write|show] <\n");
  }  
}
#endif

void
mpiosh_cmd_channel(char *args[])
{
  BYTE *channel_data, *p, name[17];
  int  size;
  int  i;
  int  chan;
  
  MPIOSH_CHECK_CONNECTION_CLOSED;

  if (args[0] != NULL) {
    if (!strcmp(args[0], "read")) {
      if ((size = mpio_file_get(mpiosh.dev, MPIO_INTERNAL_MEM, 
				MPIO_CHANNEL_FILE, NULL))<=0) {
	fprintf(stderr, "Could not read channel file\n");
      } else {
	printf("done.\n");
      }
    } else if (!strcmp(args[0], "write")) {
      printf("deleting old config file ...\n");
      mpio_file_del(mpiosh.dev, MPIO_INTERNAL_MEM, 
		    MPIO_CHANNEL_FILE, NULL);
      printf("writing new config file ...\n");
      if (mpio_file_put(mpiosh.dev, MPIO_INTERNAL_MEM, MPIO_CHANNEL_FILE,
			FTYPE_CHAN, NULL)==-1) 
	mpio_perror("error");
      mpio_sync(mpiosh.dev, MPIO_INTERNAL_MEM);
    } else if (!strcmp(args[0], "show")) {
      if ((size = mpio_file_get_to_memory(mpiosh.dev, MPIO_INTERNAL_MEM, 
				MPIO_CHANNEL_FILE, NULL, &channel_data))<=0) {
	fprintf(stderr, "Could not read channel file\n");
      } else {
	hexdump(channel_data, size);

	i=0;
	p=channel_data;
	while ((i<20) && (*p))
	  {
	    memset(name, 0, 17);
	    strncpy(name, p, 16);
	    chan = (((p[16] * 0x100) + p[17]) - 0x600)  + 1750;
	    printf("%2d. %-16s at %7.2f MHz\n", (i+1), 
		    name, ((float)chan/20));
	    p+=18;
	    i++;
	  }
	if (!i)
	  printf("no channel defined!\n");

	memcpy(channel_data, "mager", 5);

	printf("deleting old config file ...\n");
	mpio_file_del(mpiosh.dev, MPIO_INTERNAL_MEM, 
		      MPIO_CHANNEL_FILE, NULL);
	printf("writing new config file ...\n");
	if (mpio_file_put_from_memory(mpiosh.dev, MPIO_INTERNAL_MEM, 
				      MPIO_CHANNEL_FILE,
				      FTYPE_CHAN, NULL,
				      channel_data, size)==-1) 
	  mpio_perror("error");
	mpio_sync(mpiosh.dev, MPIO_INTERNAL_MEM);

	free(channel_data);
      }
    } else {
      fprintf(stderr, "unknown channel command\n");
      printf("channel [read|write|show] channel\n");
    }
  } else {
    fprintf(stderr, "error: no arguments given\n");
    printf("channel [read|write|show] <\n");
  }  
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
	if (stat((*run)->d_name, &st) == -1) {
	  perror("stat");
	} else {
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
	  if (pwd && grp) {
	    printf("%s %8s %8s %8d %10s %s\n",
		   rights, pwd->pw_name, grp->gr_name, (int)st.st_size,
		   time, (*run)->d_name);
	  } else if (pwd) {
	    printf("%s %8s %8d %8d %10s %s\n",
		   rights, pwd->pw_name, st.st_gid, (int)st.st_size,
		   time, (*run)->d_name);
	  } else if (grp) {
	    printf("%s %8d %8s %8d %10s %s\n",
		   rights, st.st_uid, grp->gr_name, (int)st.st_size,
		   time, (*run)->d_name);
	  } else {
	    printf("%s %8d %8d %8d %10s %s\n",
		   rights, st.st_uid, st.st_gid, (int)st.st_size,
		   time, (*run)->d_name);
	  }
	}
	free(*run);
      }
      free(dentry);
    } else {
      perror("scandir");
    }
  }
}

void
mpiosh_cmd_lpwd(char *args[])
{
  char	dir_buf[NAME_MAX];

  UNUSED(args);

  getcwd(dir_buf, NAME_MAX);

  printf("%s\n", dir_buf);
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

void 
mpiosh_cmd_id3(char *args[])
{  
  BYTE status;
  int n;

  MPIOSH_CHECK_CONNECTION_CLOSED;  

  if (args[0] == NULL) {
    status = mpio_id3_get(mpiosh.dev);
    printf("ID3 rewriting is %s\n", (status?"ON":"OFF"));
    return;
  } else {
    if (!strcmp(args[0], "on")) {
      status = mpio_id3_set(mpiosh.dev, 1);
    } else if (!strcmp(args[0], "off")) {
      status = mpio_id3_set(mpiosh.dev, 0);
    } else {
      fprintf(stderr, "unknown id3 command\n");
      return;
    }
    printf("ID3 rewriting is now %s\n", (status?"ON":"OFF"));
  }
}

void 
mpiosh_cmd_id3_format(char *args[])
{ 
  BYTE format[INFO_LINE];

  MPIOSH_CHECK_CONNECTION_CLOSED;

  if (args[0] == NULL) {
    mpio_id3_format_get(mpiosh.dev, format);
    printf("current format line: \"%s\"\n", format);
  } else {
    mpio_id3_format_set(mpiosh.dev, args[0]);
  }
}

void 
mpiosh_cmd_font_upload(char *args[])
{ 
  BYTE * p;
  int size;
  
  MPIOSH_CHECK_CONNECTION_CLOSED;
  MPIOSH_CHECK_ARG;

  /* check if fonts file already exists */
  p = mpio_file_exists(mpiosh.dev, MPIO_INTERNAL_MEM, 
		       MPIO_FONT_FON);
  if (p) {
    printf("Fontsfile already exists. Please delete it first!\n");
    return;
  }

  printf("writing new font file ...\n");
  if (mpio_file_put_as(mpiosh.dev, MPIO_INTERNAL_MEM, 
		       args[0], MPIO_FONT_FON,
		       FTYPE_FONT, mpiosh_callback_put)==-1) 
    mpio_perror("error");

  printf("\n");    
  mpio_sync(mpiosh.dev, MPIO_INTERNAL_MEM);
  
}


/* end of callback.c */
