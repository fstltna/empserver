/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2005, Dave Pare, Jeff Bailey, Thomas Ruschak,
 *                           Ken Stevens, Steve McClure
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  ---
 *
 *  See the "LEGAL", "LICENSE", "CREDITS" and "README" files for all the
 *  related information and legal notices. It is expected that any future
 *  projects/authors will amend these files as needed.
 *
 *  ---
 *
 *  doconfig.c: Generates the gamesdef.h file used to build the game, and
 *              the various make include files needed to build correctly.
 * 
 *  Known contributors to this file:
 *     Steve McClure, 1996-2000
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#if !defined(_WIN32)
#include <unistd.h>
#else
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#define mkdir(dir,perm) _mkdir(dir)
#endif
#include <string.h>

static void wrmakesrc(char *pathname);
static void wripglob(char *filename);
static void wrauth(char *filename);
static void wrgamesdef(char *filename);

char *copyright_header =
"/*\n"
" *  Empire - A multi-player, client/server Internet based war game.\n"
" *  Copyright (C) 1986-2005, Dave Pare, Jeff Bailey, Thomas Ruschak,\n"
" *                           Ken Stevens, Steve McClure\n"
" *\n"
" *  This program is free software; you can redistribute it and/or modify\n"
" *  it under the terms of the GNU General Public License as published by\n"
" *  the Free Software Foundation; either version 2 of the License, or\n"
" *  (at your option) any later version.\n"
" *\n"
" *  This program is distributed in the hope that it will be useful,\n"
" *  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
" *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
" *  GNU General Public License for more details.\n"
" *\n"
" *  You should have received a copy of the GNU General Public License\n"
" *  along with this program; if not, write to the Free Software\n"
" *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n"
" *\n"
" *  ---\n"
" *\n"
" *  See the \"LEGAL\", \"LICENSE\", \"CREDITS\" and \"README\" files for all the\n"
" *  related information and legal notices. It is expected that any future\n"
" *  projects/authors will amend these files as needed.\n"
" *\n"
" *  ---\n"
" *\n"
" *  %s: %s\n"
" * \n"
" *  Known contributors to this file:\n"
" *     Automatically generated by doconfig.c\n"
" */\n\n";

#if defined(__GLIBC__) || defined(FBSD) || defined(__APPLE_) || defined(_WIN32)
#define safe_getcwd() getcwd(NULL, 0)
#else
static char *
safe_getcwd(void)
{
    size_t size = 256;
     
    for (;;) {
	char *buf = malloc(size);
	if (!buf)
	    return buf;
	if (getcwd (buf, size))
	    return buf;
	free (buf);
	if (errno != ERANGE)
	    return NULL;
	size *= 2;
    }
}
#endif

int
main(void)
{
    char buf[256];
    char *cp;
    char *pathname;

    if ((pathname = safe_getcwd()) == NULL) {
	printf("Can't get current path!\n");
	exit(-1);
    }
#if !defined(_WIN32)
    cp = strrchr(pathname, '/');
    *cp = '\0';
    cp = strrchr(pathname, '/');
    *cp = '\0';
#else
    cp = strrchr(pathname, '\\');
    *cp = '\0';
    cp = strrchr(pathname, '\\');
    *cp = '\0';
#endif
    printf("Configuring...\n");
    wrmakesrc(pathname);
    sprintf(buf, "%s/include/gamesdef.h", pathname);
    wrgamesdef(buf);
    sprintf(buf, "%s/src/client/ipglob.c", pathname);
    wripglob(buf);

    if (access(EP, 0)) {
	printf("making directory %s\n", EP);
	if (mkdir(EP, 0755)) {
	    printf("mkdir failed on %s, exiting.\n", EP);
	    exit(-1);
	}
    }
    sprintf(buf, "%s/data", EP);
    if (access(buf, 0)) {
	printf("making directory %s\n", buf);
	if (mkdir(buf, 0755)) {
	    printf("mkdir failed on %s, exiting.\n", buf);
	    exit(-1);
	}
    }
    sprintf(buf, "%s/data/auth", EP);
    wrauth(buf);
    exit(0);
}

static void
wrmakesrc(char *pathname)
{
    FILE *fp;
    char buf[256];

    sprintf(buf, "%s/src/make.src", pathname);
    if ((fp = fopen(buf, "wb")) == NULL) {
	printf("Cannot open %s for writing, exiting.\n", buf);
	exit(-1);
    }
    fprintf(fp,
	    "# make.src: Source tree absolute pathname - auto generated.\n\n");
    fprintf(fp, "SRCDIR = %s\n", pathname);
    fclose(fp);
}

static void
wripglob(char *filename)
{
    FILE *fp;

    printf("Writing %s\n", filename);
    if ((fp = fopen(filename, "wb")) == NULL) {
	printf("Cannot open %s for writing, exiting.\n", filename);
	exit(-1);
    }
    fprintf(fp, copyright_header,
	    strrchr(filename, '/')+1, "IP globals.");
    fprintf(fp, "#include \"misc.h\"\n");
    fprintf(fp, "char empirehost[] = \"%s\";\n", HN);
    fprintf(fp, "char empireport[] = \"%d\";\n", PN);
    fclose(fp);
}

static void
wrauth(char *filename)
{
    FILE *fp;

    printf("Writing %s\n", filename);
    if ((fp = fopen(filename, "w")) == NULL) {
	printf("Cannot open %s for writing, exiting.\n", filename);
	exit(-1);
    }

    fprintf(fp,
	    "# %s: Empire Authorization File\n"
	    "#       Users listed will be allowed to log in as deities.\n#\n",
	    strrchr(filename, '/')+1);
    fprintf(fp, "# Format is:\n");
    fprintf(fp, "# hostname that authorized user uses on a line\n");
    fprintf(fp, "# username that authorized user uses on a line\n#\n");
    fprintf(fp, "# REMEMBER TO USE PAIRS OF LINES!\n#\n");
    fprintf(fp, "# Example:\n#\n");
    fprintf(fp, "#nowhere.land.edu\n#nowhereman\n");
    fprintf(fp, "%s\n%s\n", HN, UN);
    fprintf(fp, "%s\n%s\n", IP, UN);
    fprintf(fp, "127.0.0.1\n%s\n", UN);
    fclose(fp);
}

static void
wrgamesdef(char *filename)
{
    FILE *fp;
    char path[512];
    char *cp;
    unsigned int i;
    int s_p_etu;
    char buf[40];
    char c = 'b';

    cp = &path[0];
    for (i = 0; i < strlen(EP); i++) {
	*cp++ = EP[i];
	if (EP[i] == '\\')
	    *cp++ = '\\';
    }
    *cp = 0;

    strcpy(buf, EF);
    if (strlen(buf) > 0)
	c = buf[strlen(buf) - 1];
    if (strchr("dhm", c) && strlen(buf) > 0) {
	s_p_etu = atoi(buf);
	if (c == 'd')
	    s_p_etu =
		(((double)s_p_etu * 60.0 * 60.0 * 24.0) / (double)ET);
	else if (c == 'h')
	    s_p_etu = (((double)s_p_etu * 60.0 * 60.0) / (double)ET);
	else if (c == 'm')
	    s_p_etu = (((double)s_p_etu * 60.0) / (double)ET);
    } else {
	printf("ETU frequency is bad - using 10 minutes.\n");
	s_p_etu = 600 / ET;
    }

    printf("Writing %s\n", filename);
    if ((fp = fopen(filename, "wb")) == NULL) {
	printf("Cannot open %s for writing, exiting.\n", filename);
	exit(-1);
    }
    fprintf(fp, copyright_header,
	    strrchr(filename,'/')+1, "Server compile-time configuration");
    fprintf(fp, "/*\n * Feel free to change these, but if you rerun doconfig this file will\n");
    fprintf(fp, " * be overwritten again.\n");
    fprintf(fp, " */\n\n");
    fprintf(fp, "#ifndef _GAMESDEF_H_\n");
    fprintf(fp, "#define _GAMESDEF_H_\n\n");
    fprintf(fp, "#define EMPDIR \"%s\"\n", EP);
    fprintf(fp, "#define PRVNAM \"%s\"\n", PV);
    fprintf(fp, "#define PRVLOG \"%s\"\n", EM);
    fprintf(fp, "#define GET_SOURCE \"using:\\n    ftp://ftp.wolfpackempire.com/pub/empire/server or \\n    http://www.wolfpackempire.com/\"\n");
    fprintf(fp, "#define EMP_HOST \"%s\"\n", IP);
    fprintf(fp, "#define EMP_PORT \"%d\"\n\n", PN);
    fprintf(fp, "#define MAXNOC %d\n\n", MC);
    fprintf(fp, "#define DEF_WORLD_X %d\n", WX);
    fprintf(fp, "#define DEF_WORLD_Y %d\n\n", WY);
    fprintf(fp, "#define DEF_S_P_ETU %d\n", s_p_etu);
    fprintf(fp, "#define ETUS %d\n\n", ET);
    if (BL)
	fprintf(fp, "#define BLITZ  1\n\n");
    fprintf(fp, "#endif /* _GAMESDEF_H_ */\n");
    fclose(fp);
}
