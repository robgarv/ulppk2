/*
 *****************************************************************

<GPL>

Copyright: © 2001-2015 Robert C Garvey

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 .
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 .
 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
X-Comment: On Debian systems, the complete text of the GNU General Public
 License can be found in `/usr/share/common-licenses/GPL-3'.

</GPL>
*********************************************************************
*/

/*
 * pathinfo.c
 *
 *  Created on: Dec 3, 2012
 *      Author: robgarv
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pathinfo.h>
#include <dqacc.h>

static void push_token(PDQHEADER dequep, char* tokenp) {
	char* cp;
	cp = tokenp;
	while (*cp != '\0') {
		dq_abd(dequep, cp);
		cp++;
	}
}

static void set_dirpath(PATHINFO_STRUCT* pathinfop, PDQHEADER dequep) {
	char* cp;
	DQSTATS statsbuff;
	DQSTATS* dq_statsp;

	// Allocate a suitably sized buffer for the directory
	// path part of the pathname. This has been loaded
	// into the deque.
	dq_statsp = dq_stats(dequep, &statsbuff);
	if (dq_statsp->dquse > 0) {
		pathinfop->dirpath = (char*)calloc(dq_statsp->dquse + 1, sizeof(char));
		cp = pathinfop->dirpath;

		// Pop chars from front of queue and copy into the buffer.
		while (! dq_rtd(dequep, cp)) {
			cp++;
		}
	}
}
char* findext(char* fnbase, char* fnpart) {
	char* tokenp;
	char* ext;

	tokenp = strtok(NULL, ".");
	if ((tokenp == NULL) && (strlen(fnbase) > 0)) {
		ext = fnpart;
	} else {
		if (strlen(fnbase) != 0) {
			strcat(fnbase, ".");
		}
		strcat(fnbase, fnpart);
		ext = findext(fnbase, tokenp);
	}
	return ext;
}

static void parse_filename(PATHINFO_STRUCT* pathinfop, char* filename) {
	char* buff;
	char* fn;
	char* ext;
	int ishiddenfile = 0;
	size_t fnbufflen;
	char* fnbuff;

	if ((filename == NULL) || (strlen(filename) == 0)) {
		return;
	}
	buff = strdup(filename);
	if (filename[0] == '.') {
		// Hidden file with leading period
		ishiddenfile = 1;
	}
	fn = strtok(buff, ".");
	if (fn != NULL) {
		fnbufflen = strlen(filename);
		fnbuff = (char*)calloc(fnbufflen, sizeof(char));
		ext = findext(fnbuff, fn);
		if (ext != NULL) {
			pathinfop->extension = strdup(ext);
		}
		if (ishiddenfile) {
			char* fnbasebuff;

			fnbasebuff = (char*)calloc(strlen(fn) + 2, sizeof(char));
			strcpy(fnbasebuff, ".");
			strcat(fnbasebuff, fnbuff);
			pathinfop->basename = fnbasebuff;
		} else {
			pathinfop->basename = strdup(fnbuff);
		}
		pathinfop->filename = strdup(filename);
	}
	free(buff);
}

/*
 * Retrieves the mode field of the stat structure and
 * writes it to the caller provided memory pointed to by
 * modep. Returns non-zero on failure, zero on success.
 */
int pathinfo_get_mode(const char* pathname, mode_t* modep) {
	struct stat statbuff;
	int retval;

	retval = stat(pathname, &statbuff);
	if (retval == 0) {
		memcpy(modep, &statbuff.st_mode, sizeof(mode_t));
	}
	return retval;
}
/*
 * Returns true if the given pathname refers to a directory
 */
int pathinfo_is_dir(const char* pathname) {
	mode_t mode;
	int retval = 0;
	int status;

	status = pathinfo_get_mode(pathname, &mode);
	if (status == 0) {
		retval = S_ISDIR(mode);
	}
	return retval;
}

/*
 * Returns true if the given pathname refers to a regular file
 */
int pathinfo_is_file(const char* pathname) {
	mode_t mode;
	int retval = 0;
	int status;

	status = pathinfo_get_mode(pathname, &mode);
	if (status == 0) {
		retval = S_ISREG(mode);
	}
	return retval;
}

/*
 * Returns true if the given pathname refers to a link
 */
int pathinfo_is_link(const char* pathname) {
	mode_t mode;
	int retval = 0;
	int status;

	status = pathinfo_get_mode(pathname, &mode);
	if (status == 0) {
		retval = S_ISLNK(mode);
	}
	return retval;
}

/*
 * Returns true if the given pathname refers to a character
 * special file.
 */
int pathinfo_is_char_special(const char* pathname) {
	mode_t mode;
	int retval = 0;
	int status;

	status = pathinfo_get_mode(pathname, &mode);
	if (status == 0) {
		retval = S_ISCHR(mode);
	}
	return retval;
}

/*
 * Returns true if the given pathname refers to a block special file
 */
int pathinfo_is_block_special(const char* pathname) {
	mode_t mode;
	int retval = 0;
	int status;

	status = pathinfo_get_mode(pathname, &mode);
	if (status == 0) {
		retval = S_ISBLK(mode);
	}
	return retval;
}

/*
 * Returns true if the given pathname refers to a pipe or FIFO
 */
int pathinfo_is_pipe(const char* pathname) {
	mode_t mode;
	int retval = 0;
	int status;

	status = pathinfo_get_mode(pathname, &mode);
	if (status == 0) {
		retval = S_ISFIFO(mode);
	}
	return retval;
}

/*
 * Returns true if the given pathname refers to a socket
 */
int pathinfo_is_socket(const char* pathname) {
	mode_t mode;
	int retval = 0;
	int status;

	status = pathinfo_get_mode(pathname, &mode);
	if (status == 0) {
		retval = S_ISSOCK(mode);
	}
	return retval;
}


/*
 * Parse a file path and return a structure containing its
 * components.
 *
 * This function takes a file path (e.g. /var/tmp/myexample.txt)
 * and produces a structure containing points to the path elements.
 *
 * typedef struct _PATHINFO_STRUCT {
 *	char* fullpath;
 *	char* filename;
 *	char* dirpath;
 *	char* extension;
 *	int isdir;
 * } PATHINFO_STRUCT;
 *
 * The PATHINFO_STRUCT structure is allocated from the heap, as are
 * the strings pointed two by its members. This memory must be
 * returned to the heap by calling pathinfo_release, passing a pointer
 * to the PATHINFO_STRUCT returned by pathinfo_parse.
 */
PATHINFO_STRUCT* pathinfo_parse_filepath(const char* pathname) {
	DQHEADER deque;
	PATHINFO_STRUCT* pathinfop;
	char* pathdelim = "/";
	char* pathbuff;
	char* tokenp = NULL;
	char* token_aheadp = NULL;
	int isdir = 0;

	// Get a pathinfo structure
	pathinfop = calloc(1, sizeof(PATHINFO_STRUCT));
	pathinfop->fullpath = strdup(pathname);
	// Initialize a plenty big deque to hold path components.
	dq_init(strlen(pathname) + 1, sizeof(char), &deque);

	// Break the file path into tokens. Copy to a working buffer
	// so we don't alter path with calls to strtok
	pathbuff = strdup(pathname);

	// Check for leading /
	if (pathbuff[0] == '/') {
		push_token(&deque, "/");
	}
	isdir = pathinfo_is_dir(pathname);
	pathinfop->isdir = isdir;

	// Get first token and look ahead token
	tokenp = strtok(pathbuff, pathdelim);
	token_aheadp = strtok(NULL, pathdelim);
	while (tokenp != NULL) {
		if (token_aheadp != NULL) {
			// More tokens in path string
			push_token(&deque, tokenp);
			push_token(&deque, "/");
			tokenp = token_aheadp;
			token_aheadp = strtok(NULL, pathdelim);
		} else {
			// tokenp is the last token in the sequence
			if (!isdir) {
				parse_filename(pathinfop, tokenp);
			} else {
				push_token(&deque, tokenp);
			}
			tokenp = NULL;
		}
	}
	set_dirpath(pathinfop, &deque);
	free(pathbuff);
	return pathinfop;
}

/*
 * Releases the pathinfo structure and any attached resources.
 */
void pathinfo_release(PATHINFO_STRUCT* pathinfop) {
	if (pathinfop->basename != NULL) {
		free(pathinfop->basename);
	}
	if (pathinfop->dirpath != NULL) {
		free(pathinfop->dirpath);
	}
	if (pathinfop->extension != NULL) {
		free(pathinfop->extension);
	}
	if (pathinfop->filename != NULL) {
		free(pathinfop->filename);
	}
	if (pathinfop->fullpath != NULL) {
		free(pathinfop->fullpath);
	}
	free(pathinfop);
}

/*
 * Create a formatted string representing the pathinfo structure.
 * The returned string is allocated from the heap and must be released
 * with free.
 */
char* pathinfo_tostring(PATHINFO_STRUCT* pathinfop) {
	size_t totalsize = 126;
	char* buff;
	char* isdirtag;

	if (pathinfop->basename != NULL) {
		totalsize += strlen(pathinfop->basename);
	}
	if (pathinfop->dirpath != NULL) {
		totalsize += strlen(pathinfop->dirpath);
	}
	if (pathinfop->extension != NULL) {
		totalsize += strlen(pathinfop->extension);
	}
	if (pathinfop->filename != NULL) {
		totalsize += strlen(pathinfop->filename);
	}
	if (pathinfop->fullpath != NULL) {
		totalsize += strlen(pathinfop->fullpath);
	}

	buff = (char*)calloc(totalsize, sizeof(char));
	isdirtag = (pathinfop->isdir == 0) ? "Not a directory" : "Is a directory";
	sprintf(
			buff,
			"fullpath: %s\ndirpath: %s\nisdir: %s\nfilename: %s\nbasename: %s\nextension:%s\n",
			pathinfop->fullpath,
			pathinfop->dirpath,
			isdirtag,
			pathinfop->filename,
			pathinfop->basename,
			pathinfop->extension
	);
	return buff;
}

/*
 * Utility function for appending two elements together to make a complete
 * path. The returned buffer is allocate from the heap and must be returned
 * by free.
 *
 * The path written will be of the form <path element11>/<path element2>
 * e.g.
 *
 * pelement1 = "/usr/local"
 * pelement2 = "/etc/foo.conf"
 *
 * produces /usr/local/foo.conf
 *
 */
char* pathinfo_append2path(const char* pelement1, const char* pelement2) {
	char* pathbuff;

	pathbuff = (char*)calloc((strlen(pelement1) + strlen(pelement2) + 3), sizeof(char));
	strcpy(pathbuff, pelement1);
	if (pathbuff[strlen(pelement1) - 1 ] != '/') {
		strcat(pathbuff, "/");
	}
	strcat(pathbuff, pelement2);
	return pathbuff;
}





