/* configreader and -writer 
 *
 * Source in ANSI C, except function strdup 
 * needs _BSD_SOURCE or _SVID_SOURCE defined, if compiled ANSI
 * Thomas Buntrock (bunti@tzi.de)
 *
 * Comments in configfile are allowed, but ONLY at the BEGINNING of a line
 * When reading a config then writing the same, all comments will be removed
 *
 * The groupname MUST be put exactly between two square braces, e.g. [Name]. 
 * Any Spaces in between are interpreted as part of the name. 
 *
 * Keys and their value are separated by "=". Leading and following spaces 
 * are removed.
 * Spaces right before and after the "=" are removed. Spaces in the value 
 * will remain.
 * Keys without value are accepted, BUT the "=" is MANDATORY.
 *
 * A config line MUST NOT be longer than _CFG_MAX_LINE_SIZE (default: 256)
 *
 */

#ifndef _CFG_IO_H
#define _CFG_IO_H

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <ctype.h>

#define _CFG_MAX_LINE_SIZE 256

/* define bool */
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

typedef unsigned char bool;

struct _CFG_Key {
  char *name;
  char *value;
  struct _CFG_Key* prev;
  struct _CFG_Key* next;
  struct _CFG_Group* pp;
};

struct _CFG_Group {
  char *name;
  struct _CFG_Group* prev;
  struct _CFG_Group* next;
  struct _CFG_Key* first;
  struct _CFG_Key* last;
  struct _CFG_Handle* pp;
};

struct _CFG_Handle {
  FILE *file;
  char *filename;
  struct _CFG_Group *first;
  struct _CFG_Group *last;
};


typedef struct _CFG_Key CfgKey;
typedef struct _CFG_Group CfgGroup;
typedef struct _CFG_Handle CfgHandle;
typedef int (*cfg_key_func)(CfgKey *key);

char *cfg_resolve_path(const char *filename);

/*
 * files: a list of files 
 * e.g. char *files[] = { ".blarc", ".bla", "blarc", NULL };
 *
 * paths: a list of paths 
 * e.g. char *paths[] = { "~/", "/etc/" "etc/", NULL};
 *
 * select the first path and iterates through the files,
 * then select the next path...
 *
 * returns the first match of: path/file
 */
char *cfg_find_file (char **files, char **paths);

/*
 * Save the config 
 *
 * close: close the file after saving
 *
 * returns 0 on success or -1 on failure
 */
int cfg_save (CfgHandle *handle, int close);

/*
 * Save the config with another filename
 *
 * handle      : the config handle
 * filename    : the new filename
 * reportErrors: 0 does not report errors the stderr
 * close       : close the file after saving
 *
 * returns 0 on success or -1 on failure
 */
int cfg_save_as (CfgHandle *handle, const char* filename, 
		 int reportErrors, int close);
/*
 * Read config file
 * requires the Handle to be associated to a file and being opened
 * 
 * handle: the config handle
 * close : close the file after reading
 * 
 * returns 0 on success or -1 on failure
 */
int cfg_read (CfgHandle *handle, int close);

/*
 * Open a file to read or save 
 *
 * handle      : the config handle
 * reportErrors: write errors to stderr
 * mode        : the mode the file should be opened with (man 3 fopen)
 * 
 * returns 0 on success or -1 on failure
 */
int cfg_open (CfgHandle *handle, int reportErrors, 
	      const char* mode);
/*
 * Open a file in read mode 
 * 
 * handle      : the config handle
 * filename    : the filename of the config file
 * reportErrors: write errors to stderr
 * close       : close the file after reading 
 * 
 * returns 0 on success or -1 on failure
 */
int cfg_open_read (CfgHandle *handle, const char* filename, 
		   int reportErrors, int close);
/*
 * Close the file
 *
 * h: the config handle
 * 
 * returns 0 on success or -1 if the handle was NULL or the was already closed
 */
int cfg_close (CfgHandle *h);

/*
 * Create a new empty file handle
 * 
 * returns the pointer to the new handle
 */
CfgHandle* cfg_handle_new ();

/*
 * Create a new filehandle
 * 
 * filename: the filename of the configfile
 * read    : read the config file 
 * 
 * returns the pointer to the new handle
 */
CfgHandle* cfg_handle_new_with_filename (const char* filename, int read);

/*
 * Change the filename of the config file
 * only the internal filename ist changed, to apply on disk you need to call 
 * cfg_save 
 * 
 * filename: new filename
 * 
 * returns 0 on success or -1 on failure
 */
int cfg_handle_change_filename (CfgHandle *h, const char* filename);

/*
 * Delete the handle
 * An opened file will be closed
 *
 * h: the config handle
 */
void cfg_handle_free (CfgHandle* h);

/*
 * Create a new group within the config
 * 
 * handle: the config handle
 * name  : name of the group
 * 
 * returns 0 on success or -1 on failure
 */
int  cfg_group_new (CfgHandle* handle, const char* name);

/*
 * Delete a group within the config
 * All keys within this group will be delete as well
 *
 * handle: the config handle
 * name  : the name of the group
 */
void cfg_group_free (CfgHandle* handle, const char* name);

/*
 * returns a handle to the group g if found.
 *
 * handle: the config handle
 * name  : the name of the group to search for
 */
CfgGroup* cfg_find_group(CfgHandle* h, const char* g);

/*
 * Create a new key within a group within the config
 * 
 * h  : the config handle
 * g  : name of the group
 * key: name of the key
 * 
 * returns 0 on success or -1 on failure
 */
int  cfg_key_new (CfgHandle* h, const char* g, const char* key);

/*
 * Create a new key within the group within the config and set its value 
 * 
 * h    : the config handle
 * g    : name of the group
 * key  : name of the key
 * value: value for the key
 * 
 * returns 0 on success or -1 on failure
 */
int  cfg_key_new_with_value (CfgHandle* h, const char* g, const char* key, 
			     const char* value);

/*
 * Set the value for a specific key within a group of the config
 * 
 * h    : the config handle
 * g    : name of the group
 * key  : name of the key
 * value: value for the key
 * 
 * returns 0 on success or -1 on failure
 */
int  cfg_key_set_value (CfgHandle* h, const char* g, const char* key, 
			const char* value);

/*
 * Set the value for a specific key within a group of the config
 * 
 * h    : the config handle
 * g    : name of the group
 * key  : name of the key
 * value: value for the key
 * 
 * returns 0 on success or -1 on failure
 */
int  cfg_key_set_uint_value (CfgHandle *h, const char* g, const char* key,
			     unsigned int value);

/*
 * Set the value for a specific key within a group of the config
 * 
 * h    : the config handle
 * g    : name of the group
 * key  : name of the key
 * value: value for the key
 * 
 * returns 0 on success or -1 on failure
 */
int  cfg_key_set_int_value (CfgHandle *h, const char* g, const char* key,
			    int value);

/*
 * Set the value for a specific key within a group of the config
 * 
 * h    : the config handle
 * g    : name of the group
 * key  : name of the key
 * value: value for the key
 * 
 * returns 0 on success or -1 on failure
 */
int  cfg_key_set_double_value (CfgHandle *h, const char* g, const char* key,
			       double value);

/*
 * Set the value for a specific key within a group of the config
 * 
 * h    : the config handle
 * g    : name of the group
 * key  : name of the key
 * value: value for the key
 * 
 * returns 0 on success or -1 on failure
 */
int  cfg_key_set_float_value (CfgHandle *h, const char* g, const char* key,
			      float value);

/*
 * Set the value for a specific key within a group of the config
 * 
 * h    : the config handle
 * g    : name of the group
 * key  : name of the key
 * value: value for the key
 * 
 * returns 0 on success or -1 on failure
 */
int  cfg_key_set_bool_value (CfgHandle *h, const char* g, const char* key,
			     bool value);

/*
 * Set the value for a specific key within a group of the config
 * 
 * h    : the config handle
 * g    : name of the group
 * key  : name of the key
 * value: value for the key
 * 
 * returns 0 on success or -1 on failure
 */
int  cfg_key_set_char_value (CfgHandle *h, const char* g, const char* key,
			     char value);

/*
 * Delete a key of a group within the config
 *
 * h    : the config handle
 * g    : name of the group
 * key  : name of the key
 */
void cfg_key_free (CfgHandle* h, const char* g, const char* key);

/*
 * Get the value of a key within a group of the config
 * 
 * h  : the config handle
 * g  : name of the group
 * key: name of the key
 * res: the result will be stored in this variable
 *
 * returns 0 on success or -1 on failure
 */
int cfg_key_get_value_as_bool (CfgHandle* h, const char* g, const char* key, 
			       bool *res);

/*
 * Get the value of a key within a group of the config
 * 
 * h  : the config handle
 * g  : name of the group
 * key: name of the key
 * res: the result will be stored in this variable
 *
 * returns 0 on success or -1 on failure
 */
int cfg_key_get_value_as_uint (CfgHandle* h, const char* g, const char* key, 
			       unsigned int *res);

/*
 * Get the value of a key within a group of the config
 * 
 * h  : the config handle
 * g  : name of the group
 * key: name of the key
 * res: the result will be stored in this variable
 *
 * returns 0 on success or -1 on failure
 */
int cfg_key_get_value_as_int (CfgHandle* h, const char* g, const char* key, 
			      int *res);

/*
 * Get the value of a key within a group of the config
 * 
 * h  : the config handle
 * g  : name of the group
 * key: name of the key
 * res: the result will be stored in this variable
 *
 * returns 0 on success or -1 on failure
 */
int cfg_key_get_value_as_ushort (CfgHandle* h, const char* g, const char* key, 
				 unsigned short *res);

/*
 * Get the value of a key within a group of the config
 * 
 * h  : the config handle
 * g  : name of the group
 * key: name of the key
 * res: the result will be stored in this variable
 *
 * returns 0 on success or -1 on failure
 */
int cfg_key_get_value_as_short (CfgHandle* h, const char* g, const char* key, 
				short *res);

/*
 * Get the value of a key within a group of the config
 * 
 * h  : the config handle
 * g  : name of the group
 * key: name of the key
 * res: the result will be stored in this variable
 *
 * returns 0 on success or -1 on failure
 */
int cfg_key_get_value_as_double (CfgHandle* h, const char* g, const char* key, 
				 double *res);

/*
 * Get the value of a key within a group of the config
 * 
 * h  : the config handle
 * g  : name of the group
 * key: name of the key
 * res: the result will be stored in this variable
 *
 * returns 0 on success or -1 on failure
 */
int cfg_key_get_value_as_float (CfgHandle* h, const char* g, const char* key, 
				float *res);

/*
 * Get the value of a key within a group of the config
 * 
 * h  : the config handle
 * g  : name of the group
 * key: name of the key
 *
 * returns the value of the key or NULL if empty
 */
const char* cfg_key_get_value (CfgHandle* h, const char* g, const char* key);

/*
 * iterators through key value pairs of a group
 * 
 * h   : the config handle
 * grp : name of the group
 * func: pointer to the function to call with each element
 *
 * returns the value of the key or NULL if empty
 */
void cfg_key_for_each (CfgHandle* h, const char* grp, cfg_key_func func);
#endif /* _CFG_IO_H */
