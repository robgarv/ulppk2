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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <dqacc.h>
#include <diagnostics.h>

#include <ulppk_log.h>

/*
 **
 * @file ioutils.c
 *
 * @brief Simple low level (file descriptor) I/O utilities.
 *
 */

static char localbuff[1024 * 4];		// plenty big local buffer for formatted I/O

#define IOBLOCKSZ 256				// a "block" ... arbitrary number of bytes to operate on

#if 0
#define READ_DEQUE_SIZE (IOBLOCKSZ*4)		// make the deque size an integral number of blocks
static char read_dq_buff[READ_DEQUE_SIZE];	// a character buffer for the read deque
static char local_read_buff[IOBLOCKSZ];

// Deprecated deque based implementation stuff
DQHEADER read_deque;				// the read deque
DQHEADER* read_dequep;

static size_t fetchfromdeque(char* buff, size_t buffsize, int* eolp);
#endif

/*
 * Write n bytes to a descriptor
 * @param fd File descriptor of open file
 * @param vptr void pointer to area with data to write
 * @param n number of bytes to write out of vptr
 * @return Number of bytes read.
 */
size_t ioutils_writen(int fd, const void *vptr, size_t n) {
        size_t          nleft, nwritten;
        const char      *ptr;

        ptr = vptr;     /* can't do pointer arithmetic on void* */
        nleft = n;
        while (nleft > 0) {
                if ( (nwritten = write(fd, ptr, nleft)) <= 0)
                        return(nwritten);               /* error */

                nleft -= nwritten;
                ptr   += nwritten;
        }
        return(n);
}

/*
 * Read n bytes from a descriptor
 *
 * @param fd File descriptor of open file
 * @param vptr void pointer to area to receive data
 * @param n Max number of bytes to read into vptr
 * @return Number of bytes read.
 */
size_t ioutils_readn(int fd, void *vptr, size_t n)
{
        size_t  nleft, nread;
        char    *ptr;

        ptr = vptr;
        nleft = n;
        while (nleft > 0) {
                if ( (nread = read(fd, ptr, nleft)) < 0)
                        return(nread);          /* error, return < 0 */
                else if (nread == 0)
                        break;                          /* EOF */

                nleft -= nread;
                ptr   += nread;
        }
        return(n - nleft);              /* return >= 0 */
}

/**
 *
 * Read a string from a descriptor into the provided buffer. Do not overflow the buffer. Read
 * will be terminated after buffsize-1 characters.
 * On return, string will be null terminated. Line terminator characters ARE NOT preserved!
 * Read stops on EOF or detection of line termination.
 * @return One of the following:
 * <ul>
 * 	<li>Return value > 0 ==> number of bytes read</li>
 * 	<li>Return value < 0 ==> error</li>
 *  <li>Return value == 0 ==> EOF</li>
 * </ul>
 */
size_t ioutils_readln(int fd, char* buff, size_t buffsize) {
	int i;
	int eol;
	size_t totalread;
	size_t n2read;
	size_t nread;
	char* op;

	totalread = 0;
	eol = 0;
	n2read = buffsize - 1;

	for (i = 0, op = buff; ((i < n2read) && (!eol)); i++) {
		nread = read(fd, op, 1);
		if (nread < 0) {
			return totalread;
		} else if (nread > 0) {
			totalread++;
			if (*op == '\r') {
				// \r\n DOS line termination 1st char
				// Just skip it
				continue;
			} else if (*op == '\n') {
				// End of \r\n or \n line termination
				// Null terminate and bail
				*op = '\0';
				op++;
				return totalread;
			} else {
				op++;
				totalread++;
			}
		} else {
			// EOF detected
			break;
		}
	}
	return totalread;
}

	

/**
 * Simple minded formatted write to a file descriptor. A poor man's fprintf to 
 * a file descriptor, not a FILE* stream.
 *
 * @param fd File descriptor of an open file
 * @param fmtp printf style Format string
 * @param ... Variable length parameter list follows.
 * @return number of bytes written
 */
size_t ioutils_writef(int fd, const char* fmtp, ...) {
	va_list ap;
	int nwritten;

	va_start(ap, fmtp);
	vsprintf(localbuff, fmtp, ap);
	nwritten = ioutils_writen(fd, localbuff, strlen(localbuff));
	va_end(ap);
	return nwritten;
}

#if 0
/*
 * Deprecated.
 * Helper function reads characters stored on previous
 * read in the read deque into the buffer. Stops
 * when deque is empty, or when line terminated detected,
 * or when buffsize-1 characters have been written to buff.
 * Returns number of characters written to buff. On return, *eolp
 * will be non-zero if end of line was detected.
 */
static size_t fetchfromdeque(char* buff, size_t buffsize, int* eolp) {
	int n2read;
	int nread;			// number of bytes read into buff so far
	char* op;			// points to next byte in buff to write

	if (read_dequep == NULL) {
		read_dequep = &read_deque;
		dq_init_buffer(READ_DEQUE_SIZE, sizeof(char), read_dq_buff, read_dequep);
	}

	n2read = buffsize-1;
	memset(buff, 0, buffsize);
	op = buff;
	nread = 0;
	memset(local_read_buff, 0, sizeof(local_read_buff));

	// Pop off any previously stored characters into the output buffer
	while (
		(!dq_isempty(read_dequep))  &&
		(nread <= n2read) &&
		(*op != '\n')
	) {
		dq_rtd(read_dequep, op);
		op++;
		nread++;
	}

	if (*op == '\n') {
		// Reach an end of line
		nread++;
	}
	return nread;
}

#endif
/**
 * Determines if the specified path is an accessible
 * directory. Returns non-zero if accessible directory,
 * 0 otherwise. A diagnostic is written to the specified
 * file f_log if an error occurs. If f_log is NULL, the
 * diagnostic is written to the ulppk log.
 * @param f_log  FILE* log stream
 * @param path  presumed directory path
 * @returns  1 if directory path, 0 if not or undeterminable
 */
int ioutils_is_directory(FILE* f_log, const char* path) {
	struct stat statbuff;

	if (stat(path, &statbuff)) {
		if (NULL != f_log) {
			fprintf(f_log, "Unable to stat directory path %s\n", path);
		} else {
			ULPPK_LOG(ULPPK_LOG_ERROR, "Unable to stat directory path %s", path);
		}
		return 0;
	}

	if (!S_ISDIR(statbuff.st_mode)) {
		if (NULL != f_log) {
			fprintf(stderr, "Path is not a directory: %s", path);
		} else {
			ULPPK_LOG(ULPPK_LOG_ERROR, "Path is not a directory: %s", path);
		}
		return 0;
	}
	return 1;
}

/**
 * Determines if the specified path is an accessible
 * regular file. Returns non-zero if accessible regular file,
 * 0 otherwise. A diagnostic is written to the specified
 * file f_log if an error occurs. If f_log is NULL, the
 * diagnostic is written to the ulppk log.
 * @param f_log  FILE* log stream
 * @param file presumed regular file name or full path
 * @returns 1 if directory path, 0 if not or undeterminable
 */
int ioutils_is_regular_file(FILE* f_log, const char* file) {
	struct stat statbuff;

	if (stat(file, &statbuff)) {
		if (f_log != NULL) {
			fprintf(f_log, "Unable to stat file at path %s\n", file);
		} else {
			ULPPK_LOG(ULPPK_LOG_ERROR, "Unable to stat file at path %s", file);
		}
		return 0;
	}

	if (!S_ISREG(statbuff.st_mode)) {
		if (f_log != NULL) {
			fprintf(stderr, "Path is not a regular file: %s\n", file);
		} else {
			ULPPK_LOG(ULPPK_LOG_ERROR, "Path is not a regular file: %s", file);
		}
		return 0;
	}
	return 1;
}

/**
 * Take the directory path dirpath and the the file name fm and make a full
 * pathname out of them. Trailing slashes in dirpath are OK.
 *
 * @param buff  suitably sized buffer to receive full path (PATH_MAX to be safe)
 * @param dirpath  directory path (can have trailing slash)
 * @param fn file name
 * @returns  ptr to buff (for use in printf or other style functions taking a char*)
 */
char* ioutils_makefullpath(char* buff, const char* dirpath, const char* fn) {
	strcpy(buff, dirpath);

	if (buff[strlen(buff)-1] != '/') {
		strcat(buff,"/");
	}
	strcat(buff, fn);
	return buff;
}

/**
 * Remove a file.
 * @param filepath relative path name or full pathname of file.
 * @return 0 on success
 */
int ioutils_remove_file(char* filepath) {
	int retval = 0;

	if (ioutils_is_regular_file(NULL, filepath)) {
		retval = unlink(filepath);
	} else {
		retval = 1;
	}
	return retval;
}

/**
 * Given a file path, return the size of the file
 *
 * @param filepath Path to the file
 * @return file size in bytes
 */
size_t ioutils_file_size(char* filepath) {
	struct stat statbuff;
	size_t size = 0;
	if (ioutils_is_regular_file(NULL, filepath)) {
		stat(filepath, &statbuff);
		size = statbuff.st_size;
	}
	return size;
}
