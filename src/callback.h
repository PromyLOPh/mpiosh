/* callback.h
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

#ifndef CALLBACK_HH
#define CALLBACK_HH

#include "libmpio/mpio.h"

/* mpio command callbacks */
void mpiosh_cmd_debug(char *args[]);
void mpiosh_cmd_version(char *args[]);
void mpiosh_cmd_help(char *args[]);
void mpiosh_cmd_dir(char *args[]);
void mpiosh_cmd_pwd(char *args[]);
void mpiosh_cmd_mkdir(char *args[]);
void mpiosh_cmd_cd(char *args[]);
void mpiosh_cmd_info(char *args[]);
void mpiosh_cmd_mem(char *args[]);
void mpiosh_cmd_open(char *args[]);
void mpiosh_cmd_close(char *args[]);
void mpiosh_cmd_quit(char *args[]);
void mpiosh_cmd_get(char *args[]);
void mpiosh_cmd_mget(char *args[]);
void mpiosh_cmd_put(char *args[]);
void mpiosh_cmd_mput(char *args[]);
void mpiosh_cmd_del(char *args[]);
void mpiosh_cmd_mdel(char *args[]);
void mpiosh_cmd_dump(char *args[]);
void mpiosh_cmd_free(char *args[]);
void mpiosh_cmd_format(char *args[]);
void mpiosh_cmd_switch(char *args[]);
void mpiosh_cmd_rename(char *args[]);
void mpiosh_cmd_dump_mem(char *args[]);
void mpiosh_cmd_health(char *args[]);
void mpiosh_cmd_backup(char *args[]);
void mpiosh_cmd_restore(char *args[]);
#if 0
void mpiosh_cmd_config(char *args[]);
#endif
void mpiosh_cmd_channel(char *args[]);
void mpiosh_cmd_id3(char *args[]);
void mpiosh_cmd_id3_format(char *args[]);
void mpiosh_cmd_font_upload(char *args[]);

/* local command callbacks */
void mpiosh_cmd_ldir(char *args[]);
void mpiosh_cmd_lpwd(char *args[]);
void mpiosh_cmd_lcd(char *args[]);
void mpiosh_cmd_lmkdir(char *args[]);

/* progress callbacks */
BYTE mpiosh_callback_init(mpio_mem_t, int read, int total);
BYTE mpiosh_callback_get(int read, int total);
BYTE mpiosh_callback_put(int read, int total);
BYTE mpiosh_callback_del(int read, int total);
BYTE mpiosh_callback_format(int read, int total);

/* check situation */

#define MPIOSH_CHECK_CONNECTION_OPEN \
  if (mpiosh.dev != NULL) { \
    printf("connection to MPIO player is open\n"); \
    return; \
  }

#define MPIOSH_CHECK_CONNECTION_CLOSED \
  if (mpiosh.dev == NULL) { \
    printf("connection to MPIO player is closed\n"); \
    return; \
  }
  
#define MPIOSH_CHECK_ARG \
  if (args[0] == NULL) { \
    printf("error: no argument given\n"); \
    return; \
  }
  

#endif 

/* end of callback.h */
