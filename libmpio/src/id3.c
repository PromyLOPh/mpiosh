/*
 * $Id: id3.c,v 1.4 2003/06/12 08:32:33 germeier Exp $
 *
 *  libmpio - a library for accessing Digit@lways MPIO players
 *  Copyright (C) 2003 Markus Germeier
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc.,g 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <iconv.h>
#include <string.h>
#include <stdlib.h>

#include "id3.h"
#include "debug.h"
#include "mplib.h"
#include "mpio.h"

#ifdef MPLIB
/* local declarations */
void mpio_id3_get_content(id3_tag *, id3_tag *, int, BYTE[INFO_LINE]);
void mpio_id3_copy_tag(BYTE *, BYTE *, int *);
BYTE mpio_id3_get(mpio_t *);
BYTE mpio_id3_set(mpio_t *, BYTE);

void
mpio_id3_get_content(id3_tag *tag, id3_tag *tag2, int field, 
		       BYTE out[INFO_LINE])
{
  id3_content  *content;
  id3_text_content  *text_content;

  content = mp_get_content(tag, field);
  if (!content)
    content = mp_get_content(tag2, field);
  if (content)
    {
      text_content = mp_parse_artist(content);
      debugn(2, "Found (%d): %s\n", field, text_content->text);    
      strncpy(out, text_content->text, INFO_LINE);
    } else {
      strcpy(out,"");
    }

}

void
mpio_id3_copy_tag(BYTE *src, BYTE *dest, int *offset)
{
  int i=0;
  int last=0;
  
  /* find last non-space character, so we can strip */
  /* trailing spaces                                */
  while(src[i])
    if (src[i++]!=0x20)
      last=i;  

  i=0;
  while((*offset<(INFO_LINE-1)) && (src[i]) && (i<last))
    dest[(*offset)++]=src[i++];
}  

#endif /* MPLIB */

BYTE   
mpio_id3_set(mpio_t *m, BYTE value)
{
#ifdef MPLIB
  m->id3 = value;
  return m->id3;
#else
  return 0;
#endif /* MPLIB */
}

/* query ID3 rewriting support */
BYTE   
mpio_id3_get(mpio_t *m)
{
#ifdef MPLIB
  return m->id3;  
#else
  return 0;
#endif /* MPLIB */
}

/* ID3 rewriting: do the work */
/* context, src filename, uniq filename template */
int    
mpio_id3_do(mpio_t *m, BYTE *src, BYTE *tmp)
{
#ifdef MPLIB
  int fd, in;
  BYTE buf[BLOCK_SIZE];
  int r, w;
  int i, j, t;
  id3_tag *tag, *tag2, *new_tag;
  id3_tag_list *tag_list;
  id3_tag_list  new_tag_list;
  id3_content    new_content;
  id3v2_tag *v2_tag;  
  BYTE data_artist[INFO_LINE];
  BYTE data_title[INFO_LINE];
  BYTE data_album[INFO_LINE];
  BYTE data_year[INFO_LINE];
  BYTE data_genre[INFO_LINE];
  BYTE data_comment[INFO_LINE];
  BYTE data_track[INFO_LINE];

  BYTE mpio_tag[INFO_LINE];
  char   *mpio_tag_unicode; 

  iconv_t ic;
  int fin, fout;
  char *fback, *back;

  if (!m->id3)
    return 0;
  
  snprintf(tmp, INFO_LINE, "/tmp/MPIO-XXXXXXXXXXXXXXX");
  
  fd = mkstemp(tmp);
  if (fd==-1) return 0;
    sprintf(m->id3_temp, tmp, INFO_LINE);

  in = open(src, O_RDONLY);    
  if (in==-1) return 0;

  do {
    r=read(in, buf, BLOCK_SIZE);
    if (r>0)
      w=write(fd, buf, r);
  } while (r>0);
  
  close (in);

  tag_list = mp_get_tag_list_from_fd(fd);
  if (!tag_list)
    {
      debugn(2, "no tag list found!\n");
      return 0;
    }

  tag  = tag_list->tag;
  tag2 = NULL;
  if (tag_list->next)
    tag2 = tag_list->next->tag;

  /* read tags from file */
  mpio_id3_get_content(tag, tag2, MP_ARTIST,  data_artist);
  mpio_id3_get_content(tag, tag2, MP_TITLE,   data_title);
  mpio_id3_get_content(tag, tag2, MP_ALBUM,   data_album);
  mpio_id3_get_content(tag, tag2, MP_GENRE,   data_genre);
  mpio_id3_get_content(tag, tag2, MP_COMMENT, data_comment);
  mpio_id3_get_content(tag, tag2, MP_YEAR,    data_year);
  mpio_id3_get_content(tag, tag2, MP_TRACK,   data_track);
  
  /* build new tag */
  mpio_tag[0]=0x00;
  i=j=t=0;
  
  while ((t<(INFO_LINE-1) && m->id3_format[i]!=0))
  {
    if (m->id3_format[i] == '%') 
      {
	i++;
	switch(m->id3_format[i])
	  {
	  case 'p':
	    mpio_id3_copy_tag(data_artist, mpio_tag, &t);
	    break;
	  case 't':
	    mpio_id3_copy_tag(data_title, mpio_tag, &t);
	    break;
	  case 'a':
	    mpio_id3_copy_tag(data_album, mpio_tag, &t);
	    break;
	  case 'g':
	    mpio_id3_copy_tag(data_genre, mpio_tag, &t);
	    break;
	  case 'c':
	    mpio_id3_copy_tag(data_comment, mpio_tag, &t);
	    break;
	  case 'y':
	    mpio_id3_copy_tag(data_year, mpio_tag, &t);
	    break;
	  case 'n':
	    mpio_id3_copy_tag(data_track, mpio_tag, &t);
	    break;
	  default:
	    mpio_tag[t] = m->id3_format[i];
	  }
      } else {    
	mpio_tag[t] = m->id3_format[i];
	t++;
      }

    i++;
  }
  mpio_tag[t]=0x00;

  debugn(2, "new_tag: %s\n", mpio_tag);

  /* convert tag to UNICODELITTLE */
  fin  = strlen(mpio_tag) + 1;
  fout = fin*2 + 2;  
  ic = iconv_open("UNICODELITTLE", "ASCII");
  fback = mpio_tag;
  mpio_tag_unicode = (char *)malloc(2*INFO_LINE+3);
  back  = (char *)mpio_tag_unicode;
  *back=0x01;
  back++;
  *back=0xff;
  back++;
  *back=0xfe;
  back++;
  
  debugn(2,"iconv before %s %d %d\n", fback, fin, fout);
  iconv(ic, (char **)&fback, &fin, (char **)&back, &fout);
  debugn(2,"iconv after %s %d %d\n", fback, fin, fout);
  iconv_close(ic);
  hexdumpn(2, mpio_tag, strlen(mpio_tag));
  hexdumpn(2, (char *)mpio_tag_unicode, (2*strlen(mpio_tag))+3);

  /* build new ID3 v2 tag with only TXXX field */
  new_content.length=(2*strlen(mpio_tag))+3;
  new_content.data = (char *)malloc(new_content.length);
  new_content.compressed=0;
  new_content.encrypted=0;
  memcpy(new_content.data, mpio_tag_unicode, new_content.length);

  new_tag = mp_alloc_tag_with_version (2);
  mp_set_custom_content(new_tag, "TXXX", &new_content);

  v2_tag = (id3v2_tag *)new_tag->tag;
  v2_tag->header->unsyncronization=0;
  v2_tag->header->is_experimental=0;
  
  new_tag_list.tag   = new_tag;
  new_tag_list.next  = NULL;
  new_tag_list.first = NULL;

  /* delete ID3 v2 tag from file */
  mp_del_tags_by_ver_from_fd(fd, 2);
  close (fd);
  
  /* write new ID3 v2 tag to file */
  mp_write_to_file(&new_tag_list, tmp);

  free(mpio_tag_unicode);

  return 1;
#else
  return 0;
#endif /* MPLIB */
}

int    
mpio_id3_end(mpio_t *m) 
{
#ifdef MPLIB
  if (m->id3_temp[0])
    unlink(m->id3_temp);
  m->id3_temp[0] = 0x00;
  return 1;
#else
  return 0;
#endif /* MPLIB */
}

void   
mpio_id3_format_set(mpio_t *m, BYTE *format)
{
  strncpy(m->id3_format, format, INFO_LINE);
}
  
/* get format string for rewriting*/
void
mpio_id3_format_get(mpio_t *m, BYTE *format)
{
  strncpy(format, m->id3_format, INFO_LINE);
}
