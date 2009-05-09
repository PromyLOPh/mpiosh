/* config.c
 *
 * Author: Andreas Buesching  <crunchy@tzi.de>
 *
 * $Id: config.c,v 1.7 2003/06/27 13:40:23 crunchy Exp $
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

#include "cfg.h"
#include "global.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

struct mpiosh_config_t *
mpiosh_config_new(void)
{
  struct mpiosh_config_t * cfg = malloc(sizeof(struct mpiosh_config_t));
  char *filename, *tmp;
  struct stat st;
  
  cfg->prompt_int = cfg->prompt_ext = NULL;
  cfg->default_mem = MPIO_INTERNAL_MEM;

  filename = malloc(strlen(CONFIG_GLOBAL) + strlen(CONFIG_FILE) + 1);
  filename[0] = '\0';
  strcat(filename, CONFIG_GLOBAL);
  strcat(filename, CONFIG_FILE);
  tmp = cfg_resolve_path(filename);
  
  if (stat(tmp, &st) != -1)
    cfg->handle_global =
      cfg_handle_new_with_filename(tmp, 1);
  else
    cfg->handle_global = 0;

  free(tmp), free(filename);
  filename = malloc(strlen(CONFIG_USER) + strlen(CONFIG_FILE) + 1);
  filename[0] = '\0';
  strcat(filename, CONFIG_USER);
  strcat(filename, CONFIG_FILE);
  tmp = cfg_resolve_path(filename);

  if (stat(tmp, &st) != -1)
    cfg->handle_user =
      cfg_handle_new_with_filename(tmp, 1);
  else
    cfg->handle_user =
      cfg_handle_new_with_filename(tmp, 0);
    
  free(tmp), free(filename);

  /* initialise history */
  using_history();

  filename = malloc(strlen(CONFIG_USER) + strlen(CONFIG_HISTORY) + 1);
  filename[0] = '\0';
  strcat(filename, CONFIG_USER);
  strcat(filename, CONFIG_HISTORY);
  tmp = cfg_resolve_path(filename);

  read_history(tmp);
  free(tmp), free(filename);

  return cfg;
}

const char *
mpiosh_config_read_key(struct mpiosh_config_t *config, const char *group,
		       const char *key)
{
  char *value = NULL;
  
  if (config->handle_user)
    value = (char *)cfg_key_get_value(config->handle_user,
				      group, key);
  else if (config->handle_global)
    value = (char *)cfg_key_get_value(config->handle_global,
				      group, key);

  return value;
}

void
mpiosh_config_free(struct mpiosh_config_t *config)
{
  if (config->handle_global)
    cfg_close(config->handle_global);

  cfg_close(config->handle_user);
  free(config->prompt_int);
  free(config->prompt_ext);
  free(config);
}

int
mpiosh_config_read(struct mpiosh_config_t *config)
{
  if (config) {
    const char *value;
    
    value = mpiosh_config_read_key(config, "mpiosh", "prompt_int");
    if (value) {
      config->prompt_int = strdup(value);
    } else {
      config->prompt_int = strdup(PROMPT_INT);
    }

    value = mpiosh_config_read_key(config, "mpiosh", "prompt_ext");
    if (value) {
      config->prompt_ext = strdup(value);
    } else {
      config->prompt_ext = strdup(PROMPT_EXT);
    }

    value = mpiosh_config_read_key(config, "mpiosh", "default_mem");
    if (value) {
      if (!strcmp("internal", value)) {
	config->default_mem = MPIO_INTERNAL_MEM;
      } else if (!strcmp("external", value)) {
	config->default_mem = MPIO_EXTERNAL_MEM;
      }
    }
    
    value = mpiosh_config_read_key(config, "mpiosh", "charset");
    if (value) {
      config->charset = strdup(value);
    } else {
      config->charset = NULL;
    }

    value = mpiosh_config_read_key(config, "mpiosh", "id3_rewriting");
    if (value) {
      if (!strcmp("on", value)) {
	config->id3_rewriting = 1;
      } else  {
	config->id3_rewriting = 0;
      }
    }

    value = mpiosh_config_read_key(config, "mpiosh", "id3_format");
    if (value) {
      config->id3_format = strdup(value);
    } else {
      config->id3_format = strdup(MPIO_ID3_FORMAT);
    }
    
  }

  return 1;
}

char *
mpiosh_config_check_backup_dir( struct mpiosh_config_t *config, int create )
{
  DIR *dir;
  char *path = cfg_resolve_path( CONFIG_BACKUP );
  char *ret = path;
  
  if ( ( dir = opendir( path ) ) == NULL )
    if ( create ) {
      if ( mkdir(path, 0777 ) )
	ret = NULL;
    } else
      ret = NULL;
  else
    closedir(dir);
  
  return ret;
}

int
mpiosh_config_write( struct mpiosh_config_t *config )
{
  DIR *dir;
  char *path = cfg_resolve_path(CONFIG_USER);
  
  if ((dir = opendir(path)) == NULL)
    mkdir(path, 0777);
  else
    closedir(dir);
  
  free(path);
  
  if (config->handle_user) {
    char *tmp, *filename;

    cfg_key_set_value(config->handle_user,
		      "mpiosh", "prompt_int", config->prompt_int);
    cfg_key_set_value(config->handle_user,
		      "mpiosh", "prompt_ext", config->prompt_ext);
    if (config->default_mem == MPIO_EXTERNAL_MEM)
      cfg_key_set_value(config->handle_user,
			"mpiosh", "default_mem", "external");
    else
      cfg_key_set_value(config->handle_user,
			"mpiosh", "default_mem", "internal");
    cfg_key_set_value(config->handle_user,
		      "mpiosh", "id3_rewriting", 
		      (config->id3_rewriting?"on":"off"));
    cfg_key_set_value(config->handle_user,
		      "mpiosh", "id3_format", config->id3_format);

    cfg_save(config->handle_user, 0);

    /* save history */
    filename = malloc(strlen(CONFIG_USER) + strlen(CONFIG_HISTORY) + 1);
    filename[0] = '\0';
    strcat(filename, CONFIG_USER);
    strcat(filename, CONFIG_HISTORY);
    tmp = cfg_resolve_path(filename);

    printf("writing history to file %s\n", filename);
    write_history(tmp);
    free(tmp), free(filename);
  }
  
  return 1;
}


/* end of config.c */
