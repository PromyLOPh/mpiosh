#include "cfgio.h"

#include <sys/stat.h>
#include <unistd.h>

/***************************************************
 * help!!! functions
 ***************************************************/

char *
cfg_resolve_path(const char *filename)
{
  char *fn = NULL;

  if (filename[0] != '~') return strdup(filename);
  
  if (filename[1] == '/' ) {
    /* own home directory */
    char *home;
    if ((home = getenv ("HOME")) == NULL) {
      fn = strdup(filename);
    } else {
      fn = (char *)malloc(strlen(home) + strlen(filename+1) + 1);
      sprintf(fn, "%s%s", home,filename+1);
    }
  } else {
    /* other home directory */ 
    struct passwd *pw;
    char *dir = strchr(filename, '/');
    size_t len = (size_t)(dir - filename-1);
    char user[len];
    strncpy(user, filename+1, len);
    user[len] = '\0';
    
    if (!(pw = getpwnam(user))) {
      fn = strdup(filename); 
    } else {
      fn = (char *) malloc(strlen(pw->pw_dir) + strlen(dir) + 1);
      sprintf (fn, "%s%s",pw->pw_dir, dir);
    }  
  }  

  return fn;
}

/***************************************************
 * open functions
 ***************************************************/
int cfg_save(CfgHandle *h, int cl){
  if (h && h->file) {
    FILE *f = h->file;
    CfgGroup *group;
    group = h->first;

    while (group) {
      CfgKey *key;
      fprintf(f, "[%s]\n", group->name);
      key = group->first;
      while (key) {
	fprintf(f, "%s=%s\n", key->name, key->value ? key->value : "");
	key = key->next;
      }
      fprintf(f, "\n");
      group = group->next;
    }
    if (cl) return cfg_close(h);
    return 0;
  }
  return -1;
}

int cfg_save_as(CfgHandle *h, const char* filename, int err, int cl){
  if (h && filename) {
    int res;
    res = cfg_handle_change_filename(h, filename);
    if (res) return res;
    res = cfg_open(h, err, "w+");
    if (res) return res;
    return cfg_save(h,cl);
  }
  return -1;
}
  

int cfg_read(CfgHandle *h, int cl){
  if (h && h->file){
    char *gname = NULL;
    FILE* file = h->file;
    int offset;
    char *line = (char*)malloc(_CFG_MAX_LINE_SIZE);
    while ((line = fgets(line, _CFG_MAX_LINE_SIZE, file)) != NULL) {
      for (offset = strlen(line); offset > 0 && isspace(line[offset-1]); line[--offset] = '\0');
      for (offset = 0;isspace(line[offset]); offset++);
      switch (line[offset]) {
      case '#': 
      case '\0': break;
      case '[':	
	free (gname);
	gname = strdup(line+offset+1);
	gname[strlen(gname)-1] = '\0';
	cfg_group_new(h, gname);
	break;
      default: {
	char *k, *v, *vo, *p;
	size_t len;
	int i;
	p = strchr(line+offset, '=');
	if (p) {
	  len = (size_t)(p - (line+offset) + 1);
	  k = (char*)malloc(len);
	  k = strncpy(k, line+offset, len);
	  k[len-1] = '\0';
	  for (i = strlen(k); i > 0 && isspace(k[i-1]); k[--i] = '\0');
	  v = (char*)malloc(strlen(p+1)+1);
	  /* 	  v = (char*)malloc(strlen(p+1)); */
	  v = strcpy(v, p+1);
	  for (vo = v; isspace(vo[0]); vo++);
	  /* 	  for (vo = v+1; isspace(vo[0]); vo++); */
	  cfg_key_new_with_value(h, gname, k, vo);
	  free (v);
	} else {
	  len = (size_t)(strlen(line) - offset + 1);
	  k = (char*)malloc(len);
	  k = strncpy(k, line+offset, len);
	  k[len-1] = '\0';	    
	  vo = NULL;
	  cfg_key_new_with_value(h, gname, k, vo);
	}
	  
	free (k);
      }
      }
    }   
    free (line);
    free (gname);
    if (cl) return cfg_close(h);
    return 0;
  }
  return -1;
}


int cfg_open_file(CfgHandle* h, const char* filename, int reportErrors, const char* mode){
  if ((h->file = fopen (filename, mode)) != NULL) {
    return 0;
  } else {
    if (reportErrors) {
      fprintf (stderr, "Can't open configuration file \"%s\": %s\n",
	       filename, strerror (errno));
    }
    return -1;
  }
}

char *
cfg_find_file(char **files, char **pathes)
{
  /* e.g.:
     char *files[] = { ".blarc", "blarc", "bla", NULL };
     char *pathes[] = { "/", "/usr/etc/", "/etc/", NULL }; */ 
  char **file, **path = pathes;
  char *options_file = NULL, *help = NULL;
  struct stat s;
  
  while (*path) {
    file = files;
    while(*file) {
      free(options_file);
      options_file = (char *)malloc(sizeof(char) * (
						    strlen(*path) + 
						    strlen(*file) + 1));
      sprintf(options_file, "%s%s", *path, *file);
      help = cfg_resolve_path(options_file);
      if (!stat(help, &s)) {
	free(options_file);
	return help;
      }
      free(help);
      file++;
    }
    path++;
  }
  free(options_file);
  
  return NULL;
}

int
cfg_open_user_file(CfgHandle *h, const char* filename, 
		   int reportErrors, const char* mode)
{
  char *fn = cfg_resolve_path(filename);
  int ret = cfg_open_file(h, fn, reportErrors, mode);

  free(fn);
  
  return ret;
}

int cfg_open(CfgHandle* h, int reportErrors, const char* mode){
  int res;
  const char *filename;
  if (!h || !h->filename || !mode) return -1;
  filename = h->filename;
  if (filename[0] == '~') {
    /* Absolute Path of a Users HomeDir */
    res = cfg_open_user_file (h, filename, reportErrors, mode);
  } else if (filename[0] == '/') {
    /*       Absolute pathname: try it - there are no alternatives */
    res = cfg_open_file (h, filename, reportErrors, mode);
  } else {
    /*      Relative pathname: try current directory first */
    res = cfg_open_file (h, filename, 0, mode);
  }  
  return res;
}

int cfg_open_read (CfgHandle *handle, const char* filename, int err, int cl){
  if (handle && filename) {
    int res;
    res = cfg_handle_change_filename(handle, filename);
    if (res) return res;
    res = cfg_open(handle, err, "r");
    if (res) return res;    
    return cfg_read(handle,cl);
  }
  else return -1;
}

int cfg_close (CfgHandle* h){
  if (!h || !h->file) return -1;
  fclose (h->file);
  h->file = NULL;
  return 0;
}


/***************************************************
 * commons
 ***************************************************/

CfgGroup* cfg_find_group(CfgHandle* h, const char* g){
  CfgGroup *tmp;
  if (h == NULL || g == NULL) return NULL;
  tmp = h->first;
  while (tmp && strcmp(tmp->name, g) != 0) tmp = tmp->next;
  return tmp;
}

CfgKey* cfg_find_key(CfgGroup* g, const char* key){
  CfgKey* tmp;
  if (g == NULL || key == NULL) return NULL;
  tmp = g->first;
  while (tmp && strcmp(tmp->name, key) != 0) tmp = tmp->next;
  return tmp;
}

/***************************************************
 * cfg_handle_...
 ***************************************************/
  
CfgHandle* cfg_handle_new_with_filename (const char* filename, int read){
  CfgHandle* h;
  h = (CfgHandle*)malloc(sizeof(CfgHandle));
  if (!h) return NULL;
  
  h->filename = filename ? strdup(filename) : NULL;
  h->file  = NULL;
  h->first = h->last = NULL;

  if (read) {
    int r = cfg_open_read(h, filename, 0, 1);
    if (r != 0) {
      cfg_handle_free(h);
      return NULL;
    }
  }

  return h;
}

CfgHandle* cfg_handle_new (){
  return cfg_handle_new_with_filename(NULL,0);
}


int cfg_handle_change_filename(CfgHandle *h, const char* filename){
  if (h && filename) {
    if (h->file) return -2;
    free (h->filename);
    h->filename = strdup(filename);
    return 0;
  }
  else return -1;
}


int cfg_handle_add_group(CfgHandle* h, CfgGroup* g){
  if (h && g) {
    CfgGroup* last = h->last;
    if (last) {
      last->next = g;
      g->prev = last;
    } else {
      h->first = g;
    }
    h->last  = g;
    g->pp = h;
    return 0;
  }
  else 
    return -1;
}

void cfg_handle_free (CfgHandle* h){
  CfgGroup *ga, *gb;
  CfgKey *ka, *kb;
  if (!h) return;
  ga = h->first;
  if (h->file) fclose (h->file);
  while (ga){
    gb = ga->next;
    ka = ga->first;
    while (ka) {
      kb = ka->next;
      free (ka->name);
      free (ka->value);
      free (ka);
      ka = kb;      
    }
    free (ga->name);
    free (ga);
    ga = gb;
  }
  free (h);
}


/***************************************************
 * cfg_group_...
 ***************************************************/

int cfg_group_exist(CfgHandle* h, const char* name){
  if (h == NULL || name == NULL) 
    return -1;
  else {
    CfgGroup* tmp = h->first;
    while (tmp && strcmp(tmp->name,name) != 0 ) 
      tmp = tmp->next;
    if (tmp !=  NULL) return -1;
  }
  return 0;
}

int cfg_group_new (CfgHandle* handle, const char* name){
  if (cfg_find_group(handle, name) == NULL) {
    CfgGroup* g = (CfgGroup*)malloc(sizeof(CfgGroup));
    if (!g) return -1;
    g->name = strdup(name);
    g->prev = NULL;
    g->next = NULL;
    g->first = NULL;
    g->last = NULL;
    g->pp = NULL;
    return cfg_handle_add_group(handle, g);
  } else 
    return -1;
}

int cfg_group_add_key(CfgGroup* g, CfgKey* k){
  if (g && k) {
    CfgKey* last = g->last;
    if (last) {
      last->next = k;
      k->prev = last;
    } else {
      g->first = k;
    }
    
    g->last  = k;
    k->pp = g;
    return 0;
  } else 
    return -1;
}

void cfg_group_free(CfgHandle* h, const char* name){
  if (h && name) {
    CfgGroup *tmp, *prev, *next;
    CfgKey *a, *b;
    tmp = cfg_find_group(h, name);
    if (!tmp) return;
    
    a = tmp->first;
    while (a) {
      b = a->next;
      free (a->name);
      free (a->value);
      free (a);
      a = b;
    }

    prev = tmp->prev;
    next = tmp->next;
    
    free (tmp->name);
    if (prev) {
      prev->next = next;
    } else {
      tmp->pp->first= next;
    }

    if (next) {
      next->prev = prev;
    } else {
      tmp->pp->last = prev;
    }

    free(tmp);
  }
}

/***************************************************
 * cfg_key_...
 ***************************************************/

int
cfg_key_new (CfgHandle* h, const char* g, const char* key)
{
  CfgGroup *group;
  if ((group = cfg_find_group(h, g)) == NULL) return -1;
  //  if (cfg_find_key(group, key) != NULL) return -1;
  if (h && g && key) {
    CfgKey* k = (CfgKey*)malloc(sizeof(CfgKey));
    if (!k) return -1;
    k->name = strdup(key);
    k->value = NULL;
    k->prev = NULL;
    k->next = NULL;
    k->pp = NULL;
    return cfg_group_add_key(group, k);
  } else 
    return -1;
}

int
cfg_key_new_with_value(CfgHandle* h, const char* g, const char* key,
		       const char* value)
{
  if (cfg_key_new (h, g, key) == 0) {
    CfgKey* tmp = cfg_find_group(h, g)->last;
    if (!tmp) return -1;
    tmp->value = ((value == NULL) ? NULL : strdup(value));
    return 0;
  } else 
    return -1;
}

int
cfg_key_set_value(CfgHandle* h, const char* g, const char* key,
		  const char* value)
{
  if (h && g && key && value) {
    CfgKey* tmp = cfg_find_key(cfg_find_group(h,g), key);
    if (!tmp) return -1;
    free (tmp->value);
    tmp->value = ((value == NULL) ? NULL : strdup(value));
    return 0;
  } else return -1;
}

int
cfg_key_set_uint_value (CfgHandle *h, const char* g, const char* key,
			unsigned int value)
{
  char dest[30];
  sprintf(dest,"%ud", value);
  return cfg_key_set_value(h, g, key, dest);
}

int
cfg_key_set_int_value (CfgHandle *h, const char* g, const char* key,
		       int value)
{
  char dest[30];
  sprintf(dest,"%d", value);
  return cfg_key_set_value(h, g, key, dest);  
}

int
cfg_key_set_double_value (CfgHandle *h, const char* g, const char* key,
			  double value)
{
  char dest[30];
  sprintf(dest,"%e",value);
  return cfg_key_set_value(h, g, key, dest);
}

int
cfg_key_set_float_value (CfgHandle *h, const char* g, const char* key,
			 float value)
{	
  char dest[30];
  sprintf(dest,"%f",value);
  return cfg_key_set_value(h, g, key, dest);
}

int
cfg_key_set_bool_value (CfgHandle *h, const char* g, const char* key,
			bool value)
{
  char dest[2];
  sprintf(dest, "%1d", value);
  return cfg_key_set_value(h, g, key, dest);
}

int
cfg_key_set_char_value (CfgHandle *h, const char* g, const char* key,
			char value)
{
  char dest[2];
  sprintf(dest, "%c",value);
  return cfg_key_set_value(h, g, key, dest);  
}

const char*
cfg_key_get_value(CfgHandle* h, const char* g, const char* key)
{
  if (h && g && key) {
    CfgKey* tmp = cfg_find_key(cfg_find_group(h,g), key);
    if (!tmp) return NULL;
    return tmp->value;
  } else
    return NULL;
}

int 
cfg_key_get_value_as_bool(CfgHandle* h, const char* g, const char* key, 
			  bool* res){
  char *endptr;
  const char *nptr = cfg_key_get_value(h, g, key);
  if (!nptr) return -1;
  if (!strcasecmp(nptr, "yes") || !strcasecmp(nptr, "true"))
    *res = TRUE;
  else
    *res = FALSE;

  return (int)*endptr;
}

int
cfg_key_get_value_as_uint(CfgHandle* h, const char* g, const char* key, 
			  unsigned int* res){
  char *endptr;
  const char *nptr = cfg_key_get_value(h, g, key);
  if (!nptr) return -1;
  *res = (unsigned int)strtoul(nptr, &endptr, 10);
  return (int)*endptr;
}

int
cfg_key_get_value_as_int(CfgHandle* h, const char* g, const char* key,
			 int* res)
{
  char *endptr;
  const char *nptr = cfg_key_get_value(h, g, key);
  if (!nptr) return -1;
  *res = (int)strtol(nptr, &endptr, 10);
  return (int)*endptr;
}
  
int
cfg_key_get_value_as_ushort(CfgHandle* h, const char* g, const char* key,
			    unsigned short* res)
{
  char *endptr;
  const char *nptr = cfg_key_get_value(h, g, key);
  if (!nptr) return -1;
  *res = (unsigned short)strtoul(nptr, &endptr, 10);
  return (int)*endptr;
}

int 
cfg_key_get_value_as_short(CfgHandle* h, const char* g, const char* key,
			   short* res)
{
  char *endptr;
  const char *nptr = cfg_key_get_value(h, g, key);
  if (!nptr) return -1;
  *res = (short)strtol(nptr, &endptr, 10);
  return (int)*endptr;
}

int 
cfg_key_get_value_as_double(CfgHandle* h, const char* g, const char* key,
			    double* res)
{
  char *endptr;
  const char *nptr = cfg_key_get_value(h, g, key);
  if (!nptr) return -1;
  *res = (double)strtod(nptr, &endptr);
  return (int)*endptr;
}

int 
cfg_key_get_value_as_float(CfgHandle* h, const char* g, const char* key, 
			   float* res)
{
  char *endptr;
  const char *nptr = cfg_key_get_value(h, g, key);
  if (!nptr) return -1;
  *res = (float)strtod(nptr, &endptr);
  return (int)*endptr;
}

void
cfg_key_free (CfgHandle* h, const char* g, const char* key)
{
  if (h && g && key) {
    CfgKey *tmp, *prev, *next;
    tmp = cfg_find_key(cfg_find_group(h,g), key);
    if (!tmp) return;
    prev = tmp->prev;
    next = tmp->next;
    
    free (tmp->name);
    free (tmp->value);
    
    if (prev) {
      prev->next = next;
    } else {
      tmp->pp->first = next;
    }

    if (next) {
      next->prev = prev;
    } else {
      tmp->pp->last  = prev;
    }
    
    free (tmp);
  }
}

void
cfg_key_for_each (CfgHandle* h, const char* grp, cfg_key_func func)
{
  CfgGroup *tmp = cfg_find_group(h,grp);

  if (tmp && func) {
    CfgKey *k = tmp->first;      
    while(k) func(k), k = k->next;
  }
}
