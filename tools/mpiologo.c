/* Logo animation data extracting/changing program for MPIO DMG/DMK/
   DMG Plus/DMB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Yuji Touya (salmoon@users.sourceforge.net) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

typedef unsigned char  BYTE;

#define LOGO_FRAMESIZE  (128*48/8)
#define LOGO_FRAMESIZE_DMK (128*16/8)
#define LOGO_OFFSET 0x4000
#define LOGO_FRAMEMAX 255
#define LOGO_MAXSIZE (LOGO_FRAMESIZE * LOGO_FRAMEMAX)
#define LOGO_MAXSIZE_DMK (LOGO_FRAMESIZE_DMK * LOGO_FRAMEMAX)

struct mpio_config {
/*  BYTE volume;
    BYTE repeat;
    BYTE equalizer;
    BYTE bass;
    BYTE treble;
    BYTE lang;
    BYTE autopoff;
    BYTE playpos;
    BYTE sound3D;
    BYTE sleep;
    BYTE scroll;
    BYTE blight;
    BYTE autoplay;
    BYTE title;
    BYTE normalize;  */
    int intro;
    int end;
};

int logo_change (char *, int);
int logo_extract (char *, int);
void pbm_readpbminit (FILE *, int *, int *);
char pbm_getc (FILE *);
int pbm_getint (FILE *);
int pm_readmagicnumber (FILE * const);
int logo_read( BYTE **, char *, int);
int pbm_read (BYTE **, char *, int *, int *);
int pbm_write (BYTE *, char *, int, int);
int xbm_write (BYTE *, int, int);
void frame_trans (BYTE *);
void frame_revtrans (BYTE *);
int order_change(BYTE *, struct mpio_config *);
int order_revchange(BYTE **, BYTE *, BYTE *, struct mpio_config *);
int parameter_get (FILE *, struct mpio_config *);
BYTE bit_mirror (BYTE);
int bit_disp (BYTE);
void print_usage (FILE *, char *, int);
void version_etc (FILE *, const char *, const char *,
		  const char *, const char *);


int
main (int argc, char *argv[])
{
    int next_option;
    const char* const short_options = "hkcev";
    const struct option long_options[] = {
	{ "help",     0, NULL, 'h' },
	{ "dmk",      0, NULL, 'k' },
	{ "change",   0, NULL, 'c' },
	{ "extract",  0, NULL, 'e' },
	{ "version",  0, NULL, 'v' },
	{ NULL,       0, NULL, 0   }   /* Required at end of array.  */
    };
    char *filename;
    int isdmk = 0, change_mode = 0, extract_mode = 0;

    do {
	next_option = getopt_long (argc, argv, short_options,
                               long_options, NULL);
	switch (next_option) {
	case 'h':
	    print_usage (stderr, argv[0], 0);

	case 'k':   /* -k or --dmk */
	    isdmk = 1;
	    break;

	case 'c':   /* -c or --change */
	    change_mode = 1;
	    break;

	case 'e':   /* -e or --extract */
	    extract_mode = 1;
	    break;

	case 'v':
	    version_etc (stdout, "logotool", "mpio", "", "Salmoon");
	    exit(0);
	case '?':
	    print_usage (stderr, argv[0], 1);

	case -1:    /* Done with options.  */
	    break;

	default:    /* Something else: unexpected.  */
	    abort ();
	}
    }
    while ( next_option != -1 );

    if (optind == argc || (change_mode | extract_mode) == 0 ) {
	fprintf(stderr, "Too few arguments.\n");
	print_usage (stderr, argv[0], 1);
    }

    filename = argv[optind];

    if (extract_mode)  logo_extract(filename, isdmk);
    if (!extract_mode && change_mode )  logo_change(filename, isdmk);

    return (0);
}


int
logo_change(char *filename, int isdmk)
{
    FILE *fp;
    int i, total_i, total_e, total_dmk, nframedmk;
    BYTE *dip, *dep, *cp, *dkp;
    struct mpio_config mc;

    total_i = logo_read( &dip, "intro.pbm", isdmk );
    total_e = logo_read( &dep, "end.pbm", isdmk );

    mc.intro = total_i / (isdmk ? LOGO_FRAMESIZE_DMK : LOGO_FRAMESIZE);
    mc.end  = total_e / (isdmk ? LOGO_FRAMESIZE_DMK : LOGO_FRAMESIZE);

/* Change format from pbm to MPIO original */
    if (isdmk) {
	total_dmk = order_revchange( &dkp, dip, dep, &mc );
	nframedmk = mc.intro > mc.end ? mc.intro : mc.end;

	for ( i=0; i<nframedmk ;i++ ) {
	    frame_revtrans( dkp + i * LOGO_FRAMESIZE );
	}
    } else {
	for ( i=0; i<mc.intro ;i++ ) {
	    frame_revtrans( dip + i * LOGO_FRAMESIZE );
	}
	for ( i=0; i<mc.end ;i++ ) {
	    frame_revtrans( dep + i * LOGO_FRAMESIZE );
	}
    }

/* Read 0x4000 bytes from Config.dat */
    if (( fp = fopen(filename, "rb")) == NULL) {
	fprintf(stderr,"Congfig file read open error !!\n");
	exit(-1);
    }
    if (( cp = (BYTE *)malloc( sizeof(BYTE) * LOGO_OFFSET)) == NULL) {
	fprintf(stderr,"can't allocate memory!!\n");
	exit(-1);
    }
    if ( ( fread( cp, sizeof(BYTE), LOGO_OFFSET, fp )) != LOGO_OFFSET ) {
	fprintf(stderr,"Config data read error!!\n");
	exit(-1);
    }
    fclose(fp);

/* Rename Config.dat as config.old to back it up */
    if ((rename (filename,"config.old")) == -1 ) {
	fprintf(stderr,"\nConfig file rename failed.\n");
	exit(-1);
    }

/* Modify number of frames */
    *(cp+0x88) = mc.intro;
    *(cp+0x89) = mc.end;

/* Create new config.dat file */
    if (( fp = fopen(filename, "wb")) == NULL) {
        fprintf(stderr,"Congfig file open error !!\n");
        exit(-1);
    }
    if ( ( fwrite( cp, sizeof(BYTE), LOGO_OFFSET, fp )) != LOGO_OFFSET ) {
        fprintf(stderr,"Config data write error!! (1)\n");
        exit(-1);
    }

    if (isdmk) {
	if ( ( fwrite( dkp, sizeof(BYTE), total_dmk, fp )) != total_dmk ) {
	    free (dkp);
	    fprintf(stderr,"Config data write error!! (2)\n");
	    exit(-1);
	}
    } else {
	if ( ( fwrite( dip, sizeof(BYTE), total_i, fp )) != total_i ) {
	    fprintf(stderr,"Config data write error!! (2)\n");
	    exit(-1);
	}
	if ( ( fwrite( dep, sizeof(BYTE), total_e, fp )) != total_e ) {
	    fprintf(stderr,"Config data write error!! (3)\n");
	    exit(-1);
	}
    }
    fclose(fp);
    free(dip); free(dep); free(cp);
    printf("\n%s is modified.\n",filename);
    printf("Number of frames intro/end = %d/%d\n\n", mc.intro, mc.end);
    return(0);
}


int
logo_extract(char *filename, int isdmk)
{
    FILE *fp;
    BYTE *sp;
    int i, nframe, totalbytes;
    struct mpio_config mc;


    if (( fp = fopen( filename,"rb" )) == NULL) {
	fprintf(stderr,"%s read open error !!\n", filename);
	exit (-1);
    }

    parameter_get(fp, &mc);

    if ( isdmk ) {      /* case DMK */
	nframe = (mc.intro > mc.end ? mc.intro : mc.end);
    } else {            /* case OTHERS */
	nframe = mc.intro + mc.end;
    }

    totalbytes = nframe * LOGO_FRAMESIZE;

    if (( sp = (BYTE *)malloc( sizeof(BYTE) * totalbytes)) == NULL) {
	fprintf(stderr,"can't allocate memory!!\n");
	exit (-1);
    }
    if ( fseek( fp, LOGO_OFFSET, SEEK_SET )) {
	fprintf(stderr,"seek error!!\n");
	exit (-1);
    }
    if ( ( fread( sp, sizeof(BYTE), totalbytes, fp )) != totalbytes ) {
	fprintf(stderr,"Read error occured while reading logodata.\n"
		"Maybe this file was created for DMK. Try -k option.\n");
	exit (-1);
    }

    for ( i=0; i<nframe ;i++ ) {
	frame_trans( sp + i * LOGO_FRAMESIZE );
    }

    if ( isdmk ) {
	/* case DMK */
	order_change(sp, &mc);
	pbm_write(sp, "intro.pbm", 128, 16 * mc.intro);
	pbm_write(sp + LOGO_FRAMESIZE_DMK * mc.intro,
		  "end.pbm", 128, 16 * mc.end);
    } else {
	/* case OTHERS */
	pbm_write(sp, "intro.pbm", 128, 48 * mc.intro);
	pbm_write(sp + LOGO_FRAMESIZE * mc.intro,
		  "end.pbm", 128, 48 * mc.end);
    }
    printf("\nLogo data is extracted.\n");
    printf("Number of frames intro/end = %d/%d\n\n", mc.intro, mc.end);

    free(sp);
    fclose(fp);
    return (0);
}


int
order_change( BYTE *buffer, struct mpio_config *mcp )
{
    BYTE *temp, *p;
    int i, total;

    total = ( mcp->intro > mcp->end ? mcp->intro : mcp->end )
	* LOGO_FRAMESIZE;

    if (( temp = (BYTE *)malloc( sizeof(BYTE) * total)) == NULL) {
	fprintf(stderr,"can't allocate memory!!\n");
    exit (-1);
    }

    p = temp;
    for ( i=0; i< mcp->intro; i++ ) {
	memcpy(p, buffer + LOGO_FRAMESIZE * i + LOGO_FRAMESIZE_DMK,
	       LOGO_FRAMESIZE_DMK);
	p += LOGO_FRAMESIZE_DMK;
    }
    for ( i=0; i< mcp->end; i++ ) {
	memcpy(p, buffer + LOGO_FRAMESIZE * i, LOGO_FRAMESIZE_DMK);
	p += LOGO_FRAMESIZE_DMK;
    }
    memcpy(buffer, temp, LOGO_FRAMESIZE_DMK * (mcp->intro + mcp->end));

    free(temp);
    return (0);
}


int
order_revchange( BYTE **buffer, BYTE *buf_i, BYTE *buf_e,
		 struct mpio_config *mcp )
{
    int i, total;

    total = ( mcp->intro > mcp->end ? mcp->intro : mcp->end )
	* LOGO_FRAMESIZE;

    if (( *buffer = (BYTE *)malloc( sizeof(BYTE) * total)) == NULL) {
	fprintf(stderr,"can't allocate memory!!\n");
	exit (-1);
    }

    for ( i=0; i< mcp->intro; i++ ) {
	memcpy(*buffer + LOGO_FRAMESIZE * i + LOGO_FRAMESIZE_DMK, buf_i,
	       LOGO_FRAMESIZE_DMK);
	buf_i += LOGO_FRAMESIZE_DMK;
    }
    for ( i=0; i< mcp->end; i++ ) {
	memcpy(*buffer + LOGO_FRAMESIZE * i, buf_e, LOGO_FRAMESIZE_DMK);
	buf_e += LOGO_FRAMESIZE_DMK;
    }

    return (total);
}


/* copied from netpbm */

void
pbm_readpbminit( FILE *file, int *colsP, int *rowsP )
{
    /* Check magic number. */
    if ( pm_readmagicnumber(file) != ('P'*256 + '4') ) {
	    fprintf(stderr,"This is not a raw pbm file.\n");
	    exit(-1);
    }
    *colsP = pbm_getint(file);
    *rowsP = pbm_getint(file);
}
	

char
pbm_getc(FILE *file)
{
    register int ich;
    register char ch;

    ich = getc( file );
    if ( ich == EOF )
        fprintf(stderr, "EOF / read error\n" );
    ch = (char) ich;
    
    if ( ch == '#' ) {
        do {
            ich = getc( file );
            if ( ich == EOF )
                fprintf(stderr, "EOF / read error\n" );
            ch = (char) ich;
	}
        while ( ch != '\n' && ch != '\r' );
    }

    return ch;
}


int
pbm_getint(FILE *file)
{
    register char ch;
    register int i;

    do {
	ch = pbm_getc( file );
    }
    while ( ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' );

    if ( ch < '0' || ch > '9' )
        fprintf(stderr, "junk in file where an integer should be\n" );

    i = 0;
    do {
        i = i * 10 + ch - '0';
        ch = pbm_getc( file );
    }
    while ( ch >= '0' && ch <= '9' );

    return i;
}


int
pm_readmagicnumber(FILE * const ifP)
{
    int ich1, ich2;

    ich1 = getc(ifP);
    ich2 = getc(ifP);
    if (ich1 == EOF || ich2 == EOF)
        fprintf(stderr, "Error reading magic number from Netpbm image stream.  "
                  "Most often, this "
                  "means your input file is empty.\n" );

    return ich1 * 256 + ich2;
}
/* END copied from netpbm */


int
logo_read( BYTE **lpp, char *filename, int model )
{
    int totalbytes, framesize, maxsize, cols, rows, framerows;

    totalbytes = pbm_read( lpp, filename, &cols, &rows );

    framesize = (model ? LOGO_FRAMESIZE_DMK: LOGO_FRAMESIZE);
      maxsize = (model ? LOGO_MAXSIZE_DMK: LOGO_MAXSIZE);
    framerows = (model ? 16 : 48);

    if (totalbytes / framesize > LOGO_FRAMEMAX) {
	fprintf(stderr,"Warning: Too much frames. Forced to be set 255.\n");
	totalbytes = maxsize;
	rows = maxsize / cols * 8;
    }

    if ( cols != 128 ) fprintf(stderr,"Warning: Width is not 128 pixels.\n");
    if ( (rows % framerows) != 0 )
	fprintf(stderr,"Warning: Hight is not %d*N pixels.\n", framerows);

    return (totalbytes);
}


int
pbm_read( BYTE **pp, char *filename, int *colsp, int *rowsp )
{
    FILE *fp;
    int totalbytes;

    if (( fp = fopen( filename,"rb" )) == NULL) {
         fprintf(stderr,"%s open error !!\n", filename );
         exit (-1);
    }

    /* Check pbm header */
    pbm_readpbminit( fp, colsp, rowsp );
    totalbytes = *colsp * (*rowsp) / 8;

    /* Read raw pbm data */
    if (( *pp = (BYTE *)malloc( sizeof(BYTE) * totalbytes)) == NULL) {
	 fprintf(stderr,"can't allocate memory!!\n");
	 exit (-1);
    }
    if ( ( fread( *pp, sizeof(BYTE), totalbytes, fp )) != totalbytes ) {
	 fprintf(stderr,"%s read error!!\n",filename );
	 exit (-1);
    }

    printf("%s : %d x %d, %d bytes\n",
	 filename, *colsp, *rowsp, totalbytes);

    fclose(fp);
    return (totalbytes);
}


int
pbm_write(BYTE *p, char *filename, int cols, int rows)
{
    FILE *fp;
    int totalbytes;

    if (( fp = fopen( filename, "wb" )) == NULL) {
	fprintf(stderr,"%s open error !!\n", filename);
	exit (-1);
    }

    totalbytes = cols * rows / 8;

    fprintf (fp,"P4\n#Created by MPIOtools.\n%d %d\n", cols, rows);
    fwrite (p, 1, totalbytes, fp);
    fclose(fp);

    printf("%s : %d x %d, %d bytes\n", filename, cols, rows, totalbytes);

    return (0);
}


int
xbm_write(BYTE *p, int totalbytes, int nframe)
{
    int i;
    FILE *fp;

    if (( fp = fopen( "logo.xbm","w" )) == NULL) {
	fprintf(stderr,"open error !!\n");
	exit (-1);
    }

    fprintf (fp,"#define logo_width 128\n");
    fprintf (fp,"#define logo_height %d\n",48*nframe);
    fprintf (fp,"static unsigned char logo_bits[] = {");
  
    for (i=0; i<totalbytes-1 ; i++) {
	if ( (i % 8) == 0 ) fprintf(fp,"\n");
	fprintf(fp," 0x%02x,",0xff & *p++);
    }
    fprintf(fp," 0x%02x };\n",0xff & *p);
    fclose(fp);

    printf("Logo file logo.xbm created.\n");
    return (0);
}


void
frame_trans( BYTE *buffer )
{
    int x,y,bit;
    BYTE *to, *temp;

    if (( temp = (BYTE *)malloc( sizeof(BYTE) * LOGO_FRAMESIZE)) == NULL) {
	fprintf(stderr,"can't allocate memory!!\n");
	exit (-1);
    }
    to = temp;

    for ( x=0; x<6; x++) {
	for ( bit=0; bit<8; bit++ ) {
	    for ( y=0; y<16; y++) {
		*to = (( *(buffer + y*48 + x + 0)>> bit & 0x01) << 7)
		    + (( *(buffer + y*48 + x + 6)>> bit & 0x01) << 6)
		    + (( *(buffer + y*48 + x +12)>> bit & 0x01) << 5)
		    + (( *(buffer + y*48 + x +18)>> bit & 0x01) << 4)
		    + (( *(buffer + y*48 + x +24)>> bit & 0x01) << 3)
		    + (( *(buffer + y*48 + x +30)>> bit & 0x01) << 2)
		    + (( *(buffer + y*48 + x +36)>> bit & 0x01) << 1)
		    + ( *(buffer + y*48 + x +42)>> bit & 0x01);
		/* 	*to = bit_mirror(*to);     XBM needs this line    */
		to++;
	    }
	}
    }
    memcpy(buffer, temp, LOGO_FRAMESIZE);
    free(temp);
}


void
frame_revtrans( BYTE *buffer )
{
    int x,y,bit;
    BYTE *to, *temp;

    if (( temp = (BYTE *)malloc( sizeof(BYTE) * LOGO_FRAMESIZE)) == NULL) {
	fprintf(stderr,"can't allocate memory!!\n");
	exit(-1);
    }
    to = temp;

    for ( x=0; x<16; x++) {
	for ( bit=7; bit>=0; bit-- ) {
	    for ( y=0; y<6; y++) {
		*to = (( *(buffer + y*128 + x +  0)>> bit & 0x01) << 7)
		    + (( *(buffer + y*128 + x + 16)>> bit & 0x01) << 6)
		    + (( *(buffer + y*128 + x + 32)>> bit & 0x01) << 5)
		    + (( *(buffer + y*128 + x + 48)>> bit & 0x01) << 4)
		    + (( *(buffer + y*128 + x + 64)>> bit & 0x01) << 3)
		    + (( *(buffer + y*128 + x + 80)>> bit & 0x01) << 2)
		    + (( *(buffer + y*128 + x + 96)>> bit & 0x01) << 1)
		    + ( *(buffer + y*128 + x + 112)>> bit & 0x01);
		*to = bit_mirror(*to);
		to++;
	    }
	}
    }
    memcpy(buffer, temp, LOGO_FRAMESIZE);
    free(temp);
}


int
parameter_get(FILE *fp, struct mpio_config *mc)
{
    BYTE buffer[0x100];

    if ( fseek( fp, 0, SEEK_SET )) {
	fprintf(stderr,"seek error!!\n");
	exit (-1);
    }

    if ( fread( buffer, sizeof(BYTE), 0x100 , fp ) != 0x100 ) {
	fprintf(stderr,"read error!!\n");
	exit (-1);
    }  
    mc->intro = (int)buffer[0x88];
    mc->end  = (int)buffer[0x89];
    return (0);
}


BYTE
bit_mirror(BYTE indata)
{
    BYTE i, mirror = 0;

    for (i=0; i<8; i++) {
	mirror = (mirror << 1) + (0x01 & indata >> i);
    }
    return (mirror);
}


int
bit_disp(BYTE c)
{
    int i, wc;
    for (i=7; i>=0; i--) {
	wc = (c>>i)&(0x01);
	printf("%1d",wc);
    }
    printf("\n");
    return 0;
}


void
print_usage (FILE* stream, char* program_name, int exit_code)
{
    fprintf (stream, "Usage:  %s options [filename]\n", program_name);
    fprintf (stream,
	     "  -e  --extract filename   Extract logo from config file.\n"
	     "  -c  --change  filename   Change logo in config file.\n"
	     "  -k  --dmk                Specify logo type as DMK.\n"
	     "  -v  --version            Display version information.\n"
	     "  -h  --help               Display this usage information.\n");
    exit (exit_code);
}


void
version_etc (FILE *stream,
             const char *command_name, const char *package,
             const char *version, const char *authors)
{
  if (command_name)
    fprintf (stream, "%s (%s) %s\n", command_name, package, version);
  else
    fprintf (stream, "%s %s\n", package, version);
  fprintf (stream, "Written by %s.\n", authors);
  putc ('\n', stream);

  /*  fputs (_(version_etc_copyright), stream);
      putc ('\n', stream); */

  fputs ("\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n",
         stream);
}
