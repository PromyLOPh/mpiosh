/* 
 * debug.c
 *
 * Authors: Dirk Meyer  <dmeyer@tzi.de>
 *          Andreas Büsching <crunchy@tzi.de>
 *
 * $Id: debug.c,v 1.4 2003/04/27 11:01:29 germeier Exp $
 */

#include "debug.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define DCOLOR "_color"
#define DFILE "_file"
#define DSUFFIX "_debug"
#define LEVEL_HEXDUMP 5

#define CHECK_FD if (__debug_fd == NULL) return;
#define CHECK_FD_RETURN0 if (__debug_fd == NULL) return 0;

char *__debug_color = NULL;
int __debug_level = 0;
FILE *__debug_fd = NULL;

void 
debug_init(void) {
  char *env_var = malloc(strlen(DPACKAGE) + strlen(DFILE) + 1);
  const char *env;
  
  /* check debug output file */
  strcpy(env_var, DPACKAGE);
  strcat(env_var, DFILE);

  if ((env = getenv(env_var))) {
    if (__debug_fd && fileno(__debug_fd) != -1) {
      fclose(__debug_fd);
    }
    
    __debug_fd = fopen(env, "a");
    if (!__debug_fd) {
      __debug_fd = stderr;
    }
  } else {
    __debug_fd = stderr;
  }

  free(env_var);

  /* check debug level */
  env_var = malloc(strlen(DPACKAGE) + strlen(DSUFFIX) + 1);
  strcpy(env_var, DPACKAGE);
  strcat(env_var, DSUFFIX);
  
  if ((env = getenv(env_var)))
    if (isdigit(env[0]))
      __debug_level = strtol(env, NULL, 10);
    else
      __debug_level = 1;
  else
    __debug_level = -1;

  free(env_var);

  /* check debug color */
  env_var = malloc(strlen(DPACKAGE) + strlen(DCOLOR) + 1);
  
  strcpy(env_var, DPACKAGE);
  strcat(env_var, DCOLOR);
  
  if (__debug_color) free(__debug_color);
  __debug_color = NULL;
  
  if ((env = getenv(env_var))) {
    if (env[0] != '\0') {
      __debug_color = malloc(4 + strlen(env));
      sprintf(__debug_color, "\033[%sm", env);
    } else 
      __debug_color = malloc(6);
    strcpy(__debug_color, "\033[32m");
  } else {
    __debug_color = NULL;
  }
  
  free(env_var);
}

int
debug_file(char *filename)
{
  if (__debug_fd && fileno(__debug_fd) != -1) {
    fclose(__debug_fd);
  }

  __debug_fd = fopen(filename, "a");
  if (!__debug_fd) {
    perror("fopen:");
    __debug_fd = stderr;
    return 0;
  }

  return 1;
}

int
debug_level(int level)
{
  int tmp = __debug_level;
  
  __debug_level = level;
  
  return tmp;
}

int
debug_level_get(void)
{
  return __debug_level;
}


void
_hexdump (const char *package, const char* file, int line, 
	  const char* function, const char* data, int len)
{
  char buf[17];
  int i;

  CHECK_FD;

  if (_use_debug(LEVEL_HEXDUMP)) {
    fprintf (__debug_fd, "%s%s:\033[m %s(%d): %s: data=%p len=%d\n", 
	    __debug_color, package, file, line, function, data, len);
    for (i = 0; data != NULL && i < len; i++) {
      if (i % 16 == 0) 
	fprintf(__debug_fd, "\033[30m%s:\033[m %04x: ", package, i);
      fprintf(__debug_fd, "%02x ", (unsigned int)(unsigned char)data[i]);
      buf[i % 16] = (data[i] >= 32 && data[i] <= 126) ? data[i] : '.';
      buf[i % 16 + 1] = '\0';
      if (i % 4 == 3) fprintf(__debug_fd, " ");
      if (i % 16 == 15) fprintf(__debug_fd, "%s\n", buf);
    }
    if (i % 16 != 0) {
      for (; i % 16 != 0; i++) 
	fprintf (__debug_fd, (i % 4 == 3) ? "    " : "   ");
      fprintf(__debug_fd, "%s\n", buf);
    }
  }
}

void
_hexdump_n (const char *package, const int n, const char* file, int line, 
	  const char* function, const char* data, int len)
{
  char buf[17];
  int i;

  CHECK_FD;
  
  if (_use_debug(n)) {
    fprintf (__debug_fd, "%s%s:\033[m %s(%d): %s: data=%p len=%d\n", 
	    __debug_color, package, file, line, function, data, len);
    for (i = 0; data != NULL && i < len; i++) {
      if (i % 16 == 0) 
	fprintf(__debug_fd, "\033[30m%s:\033[m %04x: ", package, i);
      fprintf(__debug_fd, "%02x ", (unsigned int)(unsigned char)data[i]);
      buf[i % 16] = (data[i] >= 32 && data[i] <= 126) ? data[i] : '.';
      buf[i % 16 + 1] = '\0';
      if (i % 4 == 3) fprintf(__debug_fd, " ");
      if (i % 16 == 15) fprintf(__debug_fd, "%s\n", buf);
    }
    if (i % 16 != 0) {
      for (; i % 16 != 0; i++) 
	fprintf (__debug_fd, (i % 4 == 3) ? "    " : "   ");
      fprintf(__debug_fd, "%s\n", buf);
    }
  }
}


void
_hexdump_text (const char *text,
	       const char *package, const char *file, int line, 
	       const char *function, const char *data, int len)
{
  CHECK_FD;
  
  if (_use_debug(LEVEL_HEXDUMP)) {
    fprintf(__debug_fd, "%s%s: %s(%d): %s: %s\033[m", __debug_color, 
	   package, file, line, function, text);
    _hexdump(package, file, line, function, data, len);
  }
}



void
_error(const char *package, const char *file, int line, 
       const char *function, int fatal, const char *format, ...) 
{
  char foo[2048];
  va_list ap;

  CHECK_FD;

  va_start(ap, format);
  
  
  vsnprintf(foo, sizeof(foo) - strlen(format) - 1, format, ap);
  if (_use_debug(0)) 
    fprintf(__debug_fd, "\033[31m%s: %s(%d): %s: %s\033[m", 
	   package, file, line, function, foo);
  else
    fprintf(__debug_fd, "%s: %s(%d): %s: %s", 
	   package, file, line, function, foo);
  fflush(__debug_fd);

  if (fatal) {
    fprintf(__debug_fd, "\033[31mfatal error -- exit programm\033[m\n");
    exit(1);
  }
  
  va_end(ap);
}

void
_debug(const char *package, const char *file, int line, 
       const char *function, const char *format, ...)
{
  char foo[2048];
  va_list ap;
  va_start(ap, format);
  
  CHECK_FD;
  
  vsnprintf(foo, sizeof(foo) - strlen(format) - 1, format, ap);
  
  if (_use_debug(0)) {
    fprintf(__debug_fd, "%s%s: %s(%d): %s: %s\033[m",
	    ( __debug_color ? __debug_color : ""), 
	    package, file, line, function, foo);
    fflush(__debug_fd);
  }
  
  va_end(ap);
}

void
_debug_n(const char *package, const int n, const char *file, 
	 int line, const char *function, const char *format, ...)
{
  char foo[2048];
  va_list ap;
  va_start(ap, format);
  
  CHECK_FD;
  
  vsnprintf(foo, sizeof(foo) - strlen(format) - 1, format, ap);
  
  if (_use_debug(n)) {
    fprintf(__debug_fd, "%s%s: %s(%d): %s: %s\033[m",
	    ( __debug_color ? __debug_color : ""), 
	    package, file, line, function, foo);
    fflush(__debug_fd);
  }
  
  va_end(ap);
}

void
_octetstr(const char *package, const char *file, int line, 
	  const char *function, const uint8_t *str,
	  const unsigned int len, const char *what)
{
  CHECK_FD;
  
  if (_use_debug(LEVEL_HEXDUMP)) {
    unsigned int i;
    
    fprintf(__debug_fd, "%s%s: %s(%d): %s: ", 
	    package, file, function, line, (what?what:""));
    for (i = 0; i < len; i++) {
      if (i < (len - 1))
	fprintf(__debug_fd, "%03d.", str [i]);
      else
	fprintf(__debug_fd, "%03d", str [i]);
    }
  }
}

int
_use_debug(int level)
{
  if (__debug_level == -1) return 0;

  CHECK_FD_RETURN0;

  if (level <= __debug_level) {
    return 1;
  }
    
  return 0;
}

/* end of debug.c */





