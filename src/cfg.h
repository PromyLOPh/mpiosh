/* config.h
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

#ifndef MPIOSH_CONFIG_HH
#define MPIOSH_CONFIG_HH

#include "cfgio.h"

struct mpiosh_config_t {
  CfgHandle	*handle_global;
  CfgHandle	*handle_user;

  char	 	*prompt_int;
  char 		*prompt_ext;
  char          *charset;
  unsigned	default_mem;
};

struct mpiosh_config_t *mpiosh_config_new(void);
void mpiosh_config_free(struct mpiosh_config_t *config);

const char *mpiosh_config_read_key(struct mpiosh_config_t *config,
				   const char *group, const char *key);
int mpiosh_config_read(struct mpiosh_config_t *config);
int mpiosh_config_write(struct mpiosh_config_t *config);

char * mpiosh_config_check_backup_dir( struct mpiosh_config_t *config,
				    int create );

#endif 

/* end of config.h */

