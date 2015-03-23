// ====================================================================
//
//  qrzlib.cc
//
//  Interface library to the QRZ database distributed by AA7BQ
//
//  Access to the library functions is possible by C, C++, Tcl/Tk
//  or other means.  The library functions are "wrapped" for Tcl/Tk
//  using the software tool SWIG, available at www.swig.org.
//
//  Copyright (C) 1999 David Freese
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the 
//
//  Free Software Foundation, Inc.
//  51 Franklin Street, Fifth Floor
//  Boston, MA  02110-1301 USA.
//
//  Version 2.0
//    14 June 2000
//    Author: Dave Freese, W1HKJ
//  Internet: dfreese@intrepid.net
//
// ====================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#if defined(WIN32) && !defined(__CYGWIN__)
#else
# include <unistd.h>
# include <pwd.h>
#endif

#include "qrzlib.h"

char QRZdir[256] = "";

char *QRZpath;
char const *QRZtry[] = {
#ifdef WIN32
	"c:/Program Files/QRZ Ham Radio/CALLBK/",
	"c:/callbk/",
	"d:/callbk/",
	"e:/callbk/",
	"f:/callbk/",
	"g:/callbk/",
#else
	"~/callbk/",
	"/cdrom/callbk/",
	"/mnt/cdrom/callbk/",  "/mnt/cdrom0/callbk/",  "/mnt/cdrom1/callbk/",
	"/media/cdrom/callbk/", "/media/cdrom0/callbk/", "/media/cdrom1/callbk/",
	"~/callbk/",
	"/cdrom/callbk/",
	"/mnt/cdrom/callbk/",  "/mnt/cdrom0/callbk/",  "/mnt/cdrom1/callbk/",
	"/media/cdrom/callbk/", "/media/cdrom0/callbk/", "/media/cdrom1/callbk/",
#endif
	0 };

#ifdef HAVE_EMAIL
#include "../../qrzemail/QRZemail.cxx"
bool emailIsOpen = false;
#endif

FILE *imagefile = NULL;

// filename expand taken from fltk source
// replicated to exlude need for fltk in the library

#if defined(WIN32) || defined(__EMX__) && !defined(__CYGWIN__)
static inline int isdirsep(char c) {return c=='/' || c=='\\';}
#else
#define isdirsep(c) ((c)=='/')
#endif

size_t strlcpy(
			   char       *dst,	/* O - Destination string */
			   const char *src,	/* I - Source string */
			   size_t      size) {	/* I - Size of destination string buffer */

	size_t	srclen;		/* Length of source string */
	size --;
	srclen = strlen(src);
	if (srclen > size) srclen = size;
	memcpy(dst, src, srclen);
	dst[srclen] = '\0';
	return (srclen);
}

int filename_expand(char *to,int tolen, const char *from) {

	char temp[tolen];
	strlcpy(temp,from, tolen);
	char *start = temp;
	char *end = temp+strlen(temp);

	int ret = 0;

	for (char *a=temp; a<end; ) {	// for each slash component
		char *e; for (e=a; e<end && !isdirsep(*e); e++); // find next slash
		const char *value = 0; // this will point at substitute value
		switch (*a) {
			case '~':	// a home directory name
				if (e <= a+1) {	// current user's directory
					value = getenv("HOME");
#ifndef WIN32
				} else {	// another user's directory
					struct passwd *pwd;
					char t = *e; *(char *)e = 0;
					pwd = getpwnam(a+1);
					*(char *)e = t;
					if (pwd) value = pwd->pw_dir;
#endif
				}
				break;
			case '$':		/* an environment variable */
			{char t = *e; *(char *)e = 0; value = getenv(a+1); *(char *)e = t;}
				break;
		}
		if (value) {
			// substitutions that start with slash delete everything before them:
			if (isdirsep(value[0])) start = a;
#if defined(WIN32) || defined(__EMX__) && !defined(__CYGWIN__)
			// also if it starts with "A:"
			if (value[0] && value[1]==':') start = a;
#endif
			int t = strlen(value); if (isdirsep(value[t-1])) t--;
			if ((end+1-e+t) >= tolen) end += tolen - (end+1-e+t);
			memmove(a+t, e, end+1-e);
			end = a+t+(end-e);
			*end = '\0';
			memcpy(a, value, t);
			ret++;
		} else {
			a = e+1;
#if defined(WIN32) || defined(__EMX__) && !defined(__CYGWIN__)
			if (*e == '\\') {*e = '/'; ret++;} // ha ha!
#endif
		}
	}

	strlcpy(to, start, tolen);

	return ret;
}


char *QRZImageFilename (char *call)
{
	static char fname[80], *p, imgcall[12];
	FILE *f;
	strcpy(imgcall, call);
	p = imgcall;
	while (*p) {*p = tolower (*p); p++; }
	strcpy (fname, QRZdir);
	strcat (fname, "images/");
	strcat (fname, &imgcall[strlen(imgcall)-1]);
	strcat (fname, "/");
	strcat (fname, imgcall);
	while (fname[strlen(fname)-1] == ' ') fname[strlen(fname)-1] = 0;
	strcat (fname, ".jpg");
	f = fopen(fname, "r");
	if (f != NULL) {
		fclose (f);
		return fname;
	}
	return NULL;
}

int  checkPath( char *filename )
{
	char fname[80];
	FILE *f;
	char **pQRZpath = (char**)QRZtry;
	while (*pQRZpath) {
		strcpy( fname, *pQRZpath );
		strcat( fname, filename );
		strcat( fname, ".dat" );
		if (fname[0] == '~' || fname[0] == '$') {
			char f2name[80];
			filename_expand(f2name, 79, fname);
			strcpy (fname, f2name);
		}
		f = fopen(fname, "r" );
		if( f != NULL )  {
			fclose( f );
			QRZpath = *pQRZpath;
			if (QRZpath[0] == '~' || QRZpath[0] == '$')
				filename_expand(QRZdir, 79, QRZpath);
			else
				strcpy (QRZdir, QRZpath);
			return 1;
		}
		pQRZpath++;
	}
	QRZpath = (char*)QRZtry[0];
	return 0;
}

void SetQRZdirectory(char *dir)
{
	strcpy(QRZdir, dir);
	strcat(QRZdir, "/");
}

bool QRZ::ImageExists() {
	if (Qimagefname == NULL)
		return (hasImage = false);
	imagefile = fopen(Qimagefname, "r");
	if (imagefile) {
		fclose (imagefile);
		return (hasImage = true);
	}
	return (hasImage = false);
}

void QRZ::OpenQRZFiles( char *fname )
{
	long fsize;
	char dfname[64];
	char idxname[64];

	if( fname[0] == 0 ) {
		QRZvalid = 0;
		return;
	}

	QRZvalid = 1;

	if (*QRZdir == 0)
		if( checkPath( fname ) == 0 ) {
			QRZvalid = 0;
			return;
		}

	strcpy( dfname, QRZdir );
	strcpy( idxname, QRZdir );

	strcat( idxname, fname );
	strcat( idxname, ".idx" );
	strcat( dfname, fname );
	strcat( dfname, ".dat" );

	idxfile = fopen( idxname, "r" );
	if( idxfile == NULL ) {
		QRZvalid = 0;
		return;
	}

	fseek( idxfile, 0, SEEK_END );
	fsize = ftell( idxfile );
	rewind( idxfile );

	idxsize = fsize - 48;

	index = (char *) malloc( idxsize );

	if( index == NULL ) {
		fclose( idxfile );
		QRZvalid = 0;
		return;
	}
	memset( index, 0, idxsize );

	fread( &idxhdr.dataname, 1, 48, idxfile );
	fread( index, 1, idxsize, idxfile ) ;

	fflush( stdout );

	fclose( idxfile );

	datafile = fopen( dfname, "r" );
	if( datafile == NULL ) {
		free( index );
		QRZvalid = 0;
		return;
	}

	sscanf( idxhdr.bytesperkey, "%d", &datarecsize );
	if( datarecsize == 0 || datarecsize > 32767 ) {
		free( index );
		QRZvalid = 0;
		return;
	}

	// allocate sufficient data buffer for file read over key boundary

	data = (char *) malloc( datarecsize + 512 );
	if( data == NULL ) {
		free( index );
		QRZvalid = 0;
		return;
	}
	// fill buffer with new-lines to insure not reading past end of
	// the buffer
	memset( data, '\n', datarecsize + 512 );

	sscanf( idxhdr.keylen, "%d", &keylen );
	sscanf( idxhdr.numkeys, "%ld", &numkeys );
	top = index + idxsize - keylen;

#ifdef HAVE_EMAIL
	emailIsOpen = OpenEmailFile ();
#endif
}


QRZ::QRZ( char *fname )
{
	int len = strlen(fname);
	criteria = fname[ len - 1 ];
	OpenQRZFiles( fname );
}

QRZ::QRZ( char *fname, char c )
{
	criteria = c;
	OpenQRZFiles( fname );
}


QRZ::~QRZ()
{
	if( index != NULL ) free( index );
	if( data  != NULL ) free( data );
	if( datafile != NULL ) fclose( datafile );
	return;
}

int QRZ::CallComp( char *s1, char *s2 )
{
	static char sa[7], sb[7];
	strncpy( sb, s2, 6 );
	strncpy( sa, s1, 6 );
	sa[6] = 0;
	sb[6] = 0;

	int stest = strncasecmp( sa + 3, sb + 3, 3 );
	if( stest < 0 )
		return -1;
	if( stest > 0 )
		return 1;
	// suffix are equal
	int atest = strncasecmp( sa + 2, sb + 2, 1 );
	if( atest < 0 )
		return -1;
	if( atest > 0 )
		return 1;
	// suffix & call area are equal
	int ptest = strncasecmp( sa, sb, 2 );
	if( ptest < 0 )
		return -1;
	if( ptest > 0 )
		return 1;
	// total match of calls
	return 0;
}

char *Composite( char *s )
{
	static char newstr[7];
	int ccount = strlen(s) < 7 ? strlen(s) : 6;
	memset(newstr, ' ', 6 );
	newstr[6] = 0;
	if( isdigit( s[2] ) ) {
		for( int i = 0; i < ccount; i++ )
			newstr[i] = s[i];
	} else {
		newstr[0] = s[0];
		newstr[2] = s[1];
		for( int i = 2; i < ccount; i++ )
			newstr[i+1] = s[i];
	}
	return( newstr );
}

int QRZ::ReadDataBlock( long p )
{
	rewind( datafile );

	if ( p < 0 ) p = 0;

	if( fseek( datafile, p, SEEK_SET ) != 0 ) {
		return 1;
	}

	databytesread = fread( data, 1, datarecsize + 512, datafile );
	dataoffset = p;

	fflush( stdout);
	return 0;
}

int QRZ::FindCallsign( char *field )
{
	char composite[7],
	testcall[7];
	char *endofdata;
	int  matched = 0, iOffset;

	memset( composite, 0, 6 );
	memset( testcall, 0, 6 );
	found = 0;
	idxptr = index;

	if( strlen( field ) < 3 )  // must be a valid callsign
		return 0;

	if ( !(isdigit( field[1] ) || isdigit( field[2] ) ) )
		return 0;

	strcpy( composite, Composite( field ) );

	for( iOffset = 0; iOffset < numkeys; iOffset++, idxptr += keylen )
		if( CallComp( composite, idxptr) <= 0 )
			break;

	iOffset--;
	if (iOffset < 0) iOffset = 0;

	ReadDataBlock( datarecsize * iOffset );

	dfptr = data;
	endofdata = data + databytesread;

	endofline = strchr( dfptr, '\n' );

	if( idxptr != index ) {
		endofline = strchr( dfptr, '\n' );
		if (endofline != NULL )
			dfptr = endofline + 1;
	}

	found = 0;

	while ( !found && (dfptr < endofdata ) ) {
		memcpy( testcall, dfptr, 6 );
		if( (matched = CallComp( composite, Composite(testcall) ) ) <= 0 )
			found = 1;
		else {
			endofline = strchr( dfptr, '\n' );
			dfptr = endofline + 1;
		}
	}

	if ( matched == 0 ) {
		endofline = strchr( dfptr, '\n' );
		*endofline = 0;
		strcpy( recbuffer, dfptr );
		// check for old call referencing new call
		if (strlen(recbuffer) < 15 ) {
			dfptr = strchr( dfptr, ',' ) + 1;
			strcpy( recbuffer, dfptr );
			//      Qcall = recbuffer;
			found = -1;
		}
		else {
			found = 1;
			dfptr = endofline + 1;  // point to next record
		}
		return (found);
	}
	found = 0;
	return 0;
}

int QRZ::nextrec()
{
	if( dfptr > data + datarecsize ) {
		if( ReadDataBlock( dataoffset + (dfptr - data) ) != 0)
			return 0;
		dfptr = data;
	}

	endofline = strchr( dfptr, '\n' );
	*endofline = 0;
	strcpy( recbuffer, dfptr );
	dfptr = endofline + 1;
	if (strlen(recbuffer) < 15 ) {
		nextrec();
	}
	return 1;
}

int QRZ::NextRecord()
{
	if( nextrec() == 1 );
	return( ReadRec() );
	return 0;
}

int QRZ::FindName( char *field )
{
	char *endofdata;
	int  matched = 0, iOffset;
	char *Lname, *Fname;
	char sFname[17];
	char sLname[17];
	char sIdxName[33];
	char *cptr;

	memset( sFname, 0, 17 );
	memset( sLname, 0, 17 );
	memset( sIdxName, 0, 33 );

	if ( (cptr = strchr( field, ',' ) ) != NULL ) {
		strncpy( sLname, field, cptr - field );
		strcpy( sFname, cptr + 1 );
	} else
		strcpy( sLname, field );

	strcpy( sIdxName, sLname );
	if( strlen( sFname ) > 0 ) {
		strcat( sIdxName, " " );
		strcat( sIdxName, sFname );
	}

	found = 0;
	idxptr = index;

	for( iOffset = 0; iOffset < numkeys; iOffset++, idxptr += keylen )
		if( strncasecmp( sIdxName, idxptr, keylen ) <= 0 )
			break;

	iOffset--;
	if (iOffset < 0) iOffset = 0;

	ReadDataBlock( datarecsize * iOffset );

	dfptr = data;
	endofdata = data + databytesread;

	if( idxptr != index ) {
		endofline = strchr( dfptr, '\n' );
		if (endofline != NULL )
			dfptr = endofline + 1;
	}

	found = 0;
	while ( !found && (dfptr < endofdata ) ) {
		endofline = strchr( dfptr, '\n' );
		if( endofline == NULL || endofline > endofdata )
			break;
		if( endofline - dfptr > 14 ) {            // valid racord
			Lname = strchr( dfptr, ',' ) + 1;       // locate Lname element
			Fname = strchr( Lname, ',' ) + 1;       // locate Fname element
			if( *Fname == ',' )
				Fname++;
			else
				Fname = strchr( Fname, ',' ) + 1;
			if( (matched = strncasecmp( sLname, Lname, strlen(sLname) ) ) == 0 ) {
				if( sFname[0] == 0 )
					found = 1;
				else
					if( ( matched = strncasecmp( sFname, Fname, strlen(sFname) ) ) <= 0 )
						found = 1;
			}
		}
		if (!found && (dfptr < endofdata ) )
			dfptr = strchr( dfptr, '\n' ) + 1;    // move to next record
	}

	if ( matched == 0 ) {
		endofline = strchr( dfptr, '\n' );
		*endofline = 0;
		strcpy( recbuffer, dfptr );
		found = 1;
		dfptr = endofline + 1;  // point to next record
		return (found);
	}
	found = 0;
	return 0;
}

int QRZ::CompState( char *field, char *state, char *city )
{
	int compsize = strlen(field+2),
	chk;
	if (compsize > keylen) compsize = keylen;

	if(strlen( field ) == 2)
		return ( strncasecmp( field, state, 2 ) );

	if( (chk = strncasecmp( field, state, 2 ) ) < 0 )
		return -1;
	if( chk > 0 )
		return 1;
	chk = strncasecmp( field + 2, city, compsize);
	if (chk < 0)
		return -1;
	if (chk > 0)
		return 1;
	return 0;
}

int QRZ::FindState( char *field )
{
	char *endofdata;
	int  matched = 0, iOffset;
	char *state;
	char *city;
	int  compsize = strlen(field);

	if (compsize > keylen) compsize = keylen;

	found = 0;
	idxptr = index;

	for( iOffset = 0; iOffset < numkeys; iOffset++, idxptr += keylen )
		if( strncasecmp( field, idxptr, compsize ) <= 0 )
			break;

	iOffset--;
	if (iOffset < 0) iOffset = 0;

	ReadDataBlock( datarecsize * iOffset );

	dfptr = data;
	endofdata = data + datarecsize;

	if( idxptr != index ) {
		endofline = strchr( dfptr, '\n' );
		if (endofline != NULL )
			dfptr = endofline + 1;
	}

	found = 0;
	while ( !found && (dfptr < endofdata ) ) {
		endofline = strchr( dfptr, '\n' );
		if( endofline - dfptr > 14 ) {            // valid record

			city = dfptr;
			for( int i = 0; i < 9; i++ )             // move to city element
				city = strchr( city, ',' ) + 1;
			state = strchr( city, ',' ) + 1;         // move to state element
			matched = CompState( field, state, city );

			if( matched == 0)
				found = 1;
			else {
				endofline = strchr( dfptr, '\n' );  // no match, move to next
				dfptr = endofline + 1;
			}
		} else {
			endofline = strchr( dfptr, '\n' );    // invalid record, move to next
			dfptr = endofline + 1;
		}
	}

	if ( matched == 0 ) {
		endofline = strchr( dfptr, '\n' );
		*endofline = 0;
		strcpy( recbuffer, dfptr );
		found = 1;
		dfptr = endofline + 1;  // point to next record
		return (found);
	}
	found = 0;
	return 0;
}

int QRZ::FindZip( char *field )
{
	char *endofdata;
	int  matched = 0, iOffset;
	char *zip;

	found = 0;
	idxptr = index;

	for( iOffset = 0; iOffset < numkeys; iOffset++, idxptr += keylen )
		if( strncasecmp( field, idxptr, 5 ) <= 0 )
			break;

	iOffset--;
	if (iOffset < 0) iOffset = 0;

	ReadDataBlock( datarecsize * iOffset );

	dfptr = data;
	endofdata = data + datarecsize;

	if( idxptr != index ) {
		endofline = strchr( dfptr, '\n' );
		if (endofline != NULL )
			dfptr = endofline + 1;
	}

	found = 0;
	while ( !found && (dfptr < endofdata ) ) {
		endofline = strchr( dfptr, '\n' );

		if( endofline - dfptr > 14 ) {            // valid record
			zip = dfptr;
			for( int i = 0; i < 11; i++ )             // move to Zip element
				zip = strchr( zip, ',' ) + 1;
			if( (matched = strncasecmp( field, zip, 5 ) ) <= 0 )
				found = 1;
			else {
				endofline = strchr( dfptr, '\n' );  // no match, move to next
				dfptr = endofline + 1;
			}
		} else {
			endofline = strchr( dfptr, '\n' );    // invalid record, move to next
			dfptr = endofline + 1;
		}
	}

	if ( matched == 0 ) {
		endofline = strchr( dfptr, '\n' );
		*endofline = 0;
		strcpy( recbuffer, dfptr );
		found = 1;
		dfptr = endofline + 1;  // point to next record
		return (found);
	}
	found = 0;
	return 0;
}

int QRZ::FindRecord( char *field )
{
	if (QRZvalid == 0 ) return 0;

	switch (criteria) {
		case 'c' :
			FindCallsign( field );
			break;
		case 'n' :
			FindName( field );
			break;
		case 's' :
			FindState( field );
			break;
		case 'z' :
			FindZip( field );
	}
	return( ReadRec() );
}

static char *empty = (char*)"";


int QRZ::ReadRec()
{
	char *comma;

	if( found == 1 ) {
		Qcall = recbuffer;
		comma = strchr( Qcall, ',' );
		*comma = 0;
		Qlname = comma + 1;
		comma = strchr( Qlname, ',' );
		*comma = 0;
		Qfname = comma + 1;
		comma = strchr( Qfname, ',' );
		Qfname = comma + 1;      // skip JR field
		comma = strchr( Qfname, ',' );
		*comma = 0;
		Qdob = comma + 1;
		comma = strchr( Qdob, ',' );
		Qdob = comma + 1;        // skip MI field
		comma = strchr( Qdob, ',' );
		*comma = 0;
		Qefdate = comma + 1;
		comma = strchr( Qefdate, ',' );
		*comma = 0;
		Qexpdate = comma + 1;
		comma = strchr( Qexpdate, ',' );
		*comma = 0;
		Qmail_str = comma + 1;
		comma = strchr( Qmail_str, ',' );
		*comma = 0;
		Qmail_city = comma + 1;
		comma = strchr( Qmail_city, ',' );
		*comma = 0;
		Qmail_st = comma + 1;
		comma = strchr( Qmail_st, ',' );
		*comma = 0;
		Qmail_zip = comma + 1;
		comma = strchr( Qmail_zip, ',' );
		*comma = 0;
		Qopclass = comma + 1;
		comma = strchr( Qopclass, ',' );
		*comma = 0;
		Qp_call = comma + 1;
		comma = strchr( Qp_call, ',' );
		*comma = 0;
		Qp_class = comma + 1;
		Qp_class[1] = 0;
		Qimagefname = QRZImageFilename (GetCall());
		return( 1 );
	} else {
		Qcall = empty;
		Qlname = empty;
		Qfname = empty;
		Qdob = empty;
		Qefdate = empty;
		Qexpdate = empty;
		Qmail_str = empty;
		Qmail_city = empty;
		Qmail_st = empty;
		Qmail_zip = empty;
		Qopclass = empty;
		Qp_call = empty;
		Qp_class = empty;
		Qimagefname = NULL;
		return( 0 );
	}
}

int QRZ::GetCount( char *unknown )
{
	int matched, cnt = 0;
	char temp[40];

	if( FindRecord( unknown ) != 1 )
		return(0);
	matched = 0;
	while (matched == 0) {
		cnt++;
		NextRecord();
		switch (criteria) {
			case 'c' :
				matched = 1;
				break;
			case 'n' :
				if( strchr( unknown, ',' ) == 0 )
					matched = strcasecmp( unknown, GetLname() );
				else {
					strcpy( temp, GetLname() );
					strcat( temp, "," );
					strcat( temp, GetFname() );
					matched = strncasecmp( unknown, temp, strlen(unknown) );
				}
				break;
			case 'z' :
				matched = strncmp( unknown, GetZIP(), 5 );
				break;
			case 's' :
				matched = CompState( unknown, GetState(), GetCity() );
				break;
			default  :
				matched = 1;
		}
	}
	return cnt;
}

char * QRZ::GetCall()
{
	static char call[15];
	char *p = call;
	strcpy (call, Qcall);
	while (*p) {
		if (*p == ' ') strcpy (p, p+1);
		if (*p != ' ') p++;
	}
	return( call );
};

char * QRZ::GetLname()
{
	return( Qlname );
};

char * QRZ::GetFname()
{
	return( Qfname );
};

char * QRZ::GetDOB()
{
	return( Qdob );
};

char * QRZ::GetEFdate()
{
	return( Qefdate );
};

char * QRZ::GetEXPdate()
{
	return( Qexpdate );
};

char * QRZ::GetStreet()
{
	return( Qmail_str );
};

char * QRZ::GetCity()
{
	return( Qmail_city );
};

char * QRZ::GetState()
{
	return( Qmail_st );
};

char * QRZ::GetZIP()
{
	return( Qmail_zip );
};

char * QRZ::GetOPclass()
{
	return( Qopclass );
};

char * QRZ::GetPriorCall()
{
	return( Qp_call );
};

char * QRZ::GetPriorClass()
{
	return( Qp_class );
};

int QRZ::getQRZvalid()
{
	return( QRZvalid );
}

char * QRZ::GetImageFileName ()
{
	return (Qimagefname);
}

char * QRZ::CSV_Record()
{
	static char info[256];
	memset( info, 0, 256 );
	sprintf( info, "%s,%s,%s,%s,%s,%s,%s,%s,%s",
			GetCall(), Qopclass, Qefdate,
			Qlname, Qfname, Qmail_str, Qmail_city, Qmail_st, Qmail_zip );
	return info;
}

char *QRZ::Fmt_Record()
{
	static char info[256];
	memset( info, 0, 256 );
	sprintf( info, "%s   %s : %s\n%s %s\n%s\n%s, %s  %s\n%s\n",
			GetCall(), Qopclass, Qefdate,
			Qfname, Qlname,
			Qmail_str,
			Qmail_city, Qmail_st, Qmail_zip,
#ifdef HAVE_EMAIL
			szEmail(GetCall())
#else
			" "
#endif
			);
	return info;
}


