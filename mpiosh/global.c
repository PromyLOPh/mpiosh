/* global.c - containing global symbols for mpiosh
 *
 * Author: Andreas Buesching  <crunchy@tzi.de>
 *
 * $Id: global.c,v 1.1 2002/10/12 20:06:22 crunchy Exp $
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
#include "global.h"
#include "readline.h"

/* structure containing current state */
mpiosh_t mpiosh;

/* flag indicating a user-interrupt of the current command */
int mpiosh_cancel = 0;
int mpiosh_cancel_ack = 0;

/* prompt strings */
const char *PROMPT_INT = "\033[;1mmpio <i>\033[m ";
const char *PROMPT_EXT = "\033[;1mmpio <e>\033[m ";

mpiosh_cmd_t commands[] = {
  { "debug", NULL , "[level|file|on|off] <value>",
    "  modify debugging options",
    mpiosh_cmd_debug, NULL },
  { "ver", NULL, NULL,
    "  version of mpio package",
    mpiosh_cmd_version, NULL },
  { "help", NULL, "[<command>]",
    "  show information about known commands or just about <command>",
    mpiosh_cmd_help, mpiosh_readline_comp_cmd },
  { "dir", (char *[]){ "ls", "ll", NULL }, NULL,
    "  list content of current memory card",
    mpiosh_cmd_dir, NULL },
  { "info", NULL, NULL,
    "  show information about MPIO player",
    mpiosh_cmd_info, NULL },
  { "mem", NULL, "[i|e]",
    "  set current memory card. 'i' selects the internal and 'e'\n"
    "  selects the external memory card (smart media card)",
    mpiosh_cmd_mem, NULL },
  { "open", NULL, NULL,
    "  open connect to MPIO player",
    mpiosh_cmd_open, NULL },
  { "close", NULL, NULL,
    "  close connect to MPIO player",
    mpiosh_cmd_close, NULL },
  { "quit", (char *[]){ "exit", NULL }, NULL,
    "  exit mpiosh and close the device",
    mpiosh_cmd_quit, NULL },
  { "mget", (char *[]){ "get", NULL }, "list of filenames and <regexp>",
    "  read all files matching the regular expression\n"
    "  from the selected memory card",
    mpiosh_cmd_mget, mpiosh_readline_comp_mpio_file },
  { "mput", (char *[]){ "put", NULL }, "list of filenames and <regexp>",
    "  write all local files matching the regular expression\n"
    "  to the selected memory card",
    mpiosh_cmd_mput, mpiosh_readline_comp_mpio_file },
  { "mdel", (char *[]){ "rm", "del", NULL }, "<regexp>",
    "  deletes all files matching the regular expression\n"
    "  from the selected memory card",
    mpiosh_cmd_mdel, mpiosh_readline_comp_mpio_file },
  { "dump", NULL, NULL,
    "  get all files of current memory card",
    mpiosh_cmd_dump, NULL },
  { "free", NULL, NULL,
    "  display amount of available bytes of current memory card",
    mpiosh_cmd_free, NULL },
  { "format", NULL, NULL,
    "  format current memory card",
    mpiosh_cmd_format, NULL },
/*   { "switch", "<file1> <file2>", */
/*     "switches the order of two files", */
/*     mpiosh_cmd_switch }, */
  { "ldir", (char *[]){ "lls", NULL }, NULL,
    "  list local directory",
    mpiosh_cmd_ldir, NULL },
  { "lpwd", NULL, NULL,
    "  print current working directory",
    mpiosh_cmd_lpwd, NULL },
  { "lcd", NULL, NULL,
    "  change the current working directory",
    mpiosh_cmd_lcd, NULL },
  { "lmkdir", NULL, NULL,
    "  create a local directory",
    mpiosh_cmd_lmkdir, NULL },
  { "dump_memory", NULL, NULL,
    "  dump FAT, directory, spare area and the first 0x100 of the\n"
    "  selected memory card",
    mpiosh_cmd_dump_mem, NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL }
};

/* end of global.c */
