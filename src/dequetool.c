
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

/**
 * @file dequetool.c
 *
 * @brief Deque maintenance utility.
 *
 * dequetool provides features for manipulating memory mapped double ended queues.
 * These features include:
 * <ul>
 * <li>-c --create : Create a memory mapped double ended queue</li>
 * <li>-r --report : Report status of the memory mapped double ended queue</li>
 * <li>-z --zap : Zap (reset) a memory mapped double ended queue</li>
 * <li>-i --inject : Inject data onto the bottom of a memory mapped double ended queue</li>
 * <li>-e --extract : Extract data from the top a memory mapped double ended queue</li>
 * <li>-b --binary : For inject/extract operations use binary transfer mode
 * <li>-h --help : command line help</li>
 * <li>-d --directory : Data directory -- overrides env var MMDQ_DIR_PATH (default /var/ulppk/data)</li>
 * <li>-q --queue : Name of the double ended queue (filename w/o extension)</li>
 * <li>-f --file : Name of file that is source of data for injection (-i) or destination for extraction (-e) </li>
 * <li>-n --nitems : Number of items the dequeue can contain. (create option only) </li>
 * <li>-s --sizeofitem : Size of a deque item in bytes (create otion only)</li>
 * </ul>
 *
 * About transfer modes for the inject and extract operations. The issue revolves around deque item
 * size and the number of items required to transmit the contents of a file. To illustrate, let's
 * assum the deque item size is 5 bytes and the file we want to inject is 16 bytes long.
 * This will require 4 items ... and one of them will only contain one byte. However when we extract
 * the data, the last item popped from the deque will contain that last data byte and four trailing
 * nulls. If that is unimportant, or if item size is 1 byte, then ascii transfer mode is adequate.
 *
 * However, if item size is not one byte and a file written with extracted data MUST be of the same
 * length as the original file, binary transfer mode is necessary. Items in binary transfer mode
 * are written with a header so that only valid data bytes are written to the output file.
 *
 * Examples:
 *
 * Create a deque that can hold 20 64 byte records.  The deque is called mydeque and will be
 * implemented as a file at /var/ulppk2/memfiles/mydeque.dq
 *
 * dequetool -c -q mydeque -d /var/ulppk2/memfiles -n 20 -s 64
 *
 * Report on the deque just created.
 *
 * dequetool -r -q mydeque -d /var/ulppk2/memfiles
 *
 *
 * Inject data in file foo.txt into the deque using ascii transfer mode.
 *
 * dequetool -i -q mydeque -d /var/ulppk2/memfiles -f foo.txt
 *
 * Extract data from the deque into the file bar.txt using ascii transfer mode.
 *
 * dequetool -e -q mydeque -d /var/ulppk2/memfiles -f bar.txt
 *
 * Zap this deque ... resets it to the empty state
 *
 * dequetool -z -q mydeque -d /var/ulppk2/memfiles
 */
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

#include <appenv.h>
#include <diagnostics.h>
#include <cmdargs.h>
#include <mmdeque.h>
#include <mmrpt_deque.h>

char ebuff[2046];

static void register_args(int argc, char* argv[]) {
	
	cmdarg_init(argc, argv);
	cmdarg_register_option("c", "create", CA_SWITCH,
		"Create a memory mapped double ended queue", NULL, NULL);
	cmdarg_register_option("r", "report", CA_SWITCH,
		"Report status of memory mapped double ended queue", NULL, NULL);
	cmdarg_register_option("z", "zap", CA_SWITCH,
		"Reset (zap) a memory mapped double ended queue", NULL, NULL);
	cmdarg_register_option("i", "inject", CA_SWITCH,
		"Inject/Push the contents of a file into the deque", NULL, NULL);
	cmdarg_register_option("e", "extract", CA_SWITCH,
		"Extract/Pop the contents of a deque into a file", NULL, NULL);
	cmdarg_register_option("h", "help", CA_SWITCH,
		"Print command help", NULL, NULL);
	cmdarg_register_option("b", "binary", CA_SWITCH,
			"inject/extract operations binary transfer mode", NULL, NULL);
		
	// Common options
	cmdarg_register_option("d", "directory", CA_DEFAULT_ARG,
		"Data directory -- overrides env var MMDQ_DIR_PATH", "/var/ulppk/data", NULL); 
		
	cmdarg_register_option("q", "queue", CA_REQUIRED_ARG,
		"Deque name", NULL, NULL);
	
	cmdarg_register_option("f", "file", CA_OPTIONAL_ARG, 
		"Name of file that is source or sink for p and e options", NULL, NULL);
	cmdarg_register_option("n", "nitems", CA_OPTIONAL_ARG,
		"Number of items the deque can contain", NULL, NULL);
	cmdarg_register_option("s", "sizeofitem", CA_OPTIONAL_ARG,
		"Size of items in bytes", NULL, NULL);
	
}

static void fetch_values(char* quename, char* filepath, int* nitemsp, int* itemsizep) {
	char* qname;
	char* fpath;

	qname = cmdarg_fetch_string(NULL, "q");
	if (NULL == qname) {
		// qname is always required
		cmdarg_show_help(NULL);
		APP_ERR(stderr, "q option is always required");
	}
	strcpy(quename, qname);
	if ((cmdarg_fetch_switch(NULL, "i")) || (cmdarg_fetch_switch(NULL, "e"))) {
		fpath = cmdarg_fetch_string(NULL, "f");
		if (NULL == fpath) {
			// This is actually OK. It just means stdout (e) or stdin (i)
			strcpy(filepath, "stdio");
		} else {
			strcpy(filepath, fpath);
		}
	} else {
		if (cmdarg_fetch_switch(NULL, "c")) {
			// These options require -n and -s
			if (cmdarg_fetch_string(NULL, "n") == NULL) {
				cmdarg_show_help(NULL);
				APP_ERR(stderr, "-c/--create: Must provide -n param (number of items");
			} 
			*nitemsp = cmdarg_fetch_int(NULL, "n");
			if (cmdarg_fetch_string(NULL, "s") == NULL) {
				cmdarg_show_help(NULL);
				APP_ERR(stderr, "-c/--create: Must provide -s param (sizeofitem)");
			}
			*itemsizep = cmdarg_fetch_int(NULL, "s");
		}
	}
}
	
static int process_switch_help() {
	if (cmdarg_fetch_switch(NULL, "h")) {
		cmdarg_show_help(NULL);
		return 1;	// action taken ... stop processing arguments
	}
	return 0;		// no action take ... keep processing arguments
}

static int process_switch_d() {
	char* dirpath;
	struct stat buff;
	

	if (cmdarg_fetch_switch(NULL, "d")) {
		dirpath = cmdarg_fetch_string(NULL, "d");
		if (stat(dirpath, &buff) < 0) {
			APP_ERR(stderr, "Error accessing status of directory path %s\n%s\n", dirpath, strerror(errno));
		} else {
			if (S_ISDIR(buff.st_mode)) {
				appenv_set_env_var(MMDQ_DIR_PATH, dirpath);
			} else {
				APP_ERR(stderr, "The file path  %s does not resolve to a directory", dirpath);
			}	
		}
	}
	// Returning zero to indicate no action taken.
	return 0;
}

static int process_switch_c() {
	char deque_name[MAX_DEQUE_NAME_LEN];
	char filepath[PATH_MAX];
	int nitems;
	int itemsize;
	MMA_HANDLE* mmahp;
	
	if (cmdarg_fetch_switch(NULL, "c")) {
		fetch_values(deque_name, filepath, &nitems, &itemsize);
		mmahp = mmdq_create(deque_name, itemsize, nitems);
		if (NULL == mmahp) {
			mma_strerror(ebuff, sizeof(ebuff));
			APP_ERR(stderr, ebuff);
		}
		return 1;		// action taken ... stop processing arguments
	}
	return 0;			// no action taken
}

static int process_switch_r() {
	char deque_name[MAX_DEQUE_NAME_LEN];
	char filepath[PATH_MAX];
	int nitems;
	int itemsize;
	MMA_HANDLE* mmahp;
	
	
	if (cmdarg_fetch_switch(NULL, "r")) {
		fetch_values(deque_name, filepath, &nitems, &itemsize);
		mmahp = mmdq_open(deque_name);
		if (NULL == mmahp) {
			mma_strerror(ebuff, sizeof(ebuff));
			APP_ERR(stderr, ebuff);
		}
		mmrpt_deque2file(stdout, mmahp); 
		return 1;		// action taken ... stop processing arguments
	}
	return 0; 			// no action taken ... keep looping
}
static int process_switch_z() {
	char deque_name[MAX_DEQUE_NAME_LEN];
	char filepath[PATH_MAX];
	int nitems;
	int itemsize;
	MMA_HANDLE* mmahp;
	
	
	if (cmdarg_fetch_switch(NULL, "z")) {
		fetch_values(deque_name, filepath, &nitems, &itemsize);
		mmahp = mmdq_open(deque_name);
		mmdq_reset(mmahp);
		fprintf(stdout, "Deque %s has been reset\n", deque_name);
		return 1;		// action taken ... stop processing arguments
	}
	return 0; 			// no action taken ... keep looping
}

static int process_switch_i() {
	DQSTATS dq_stats;
	DQSTATS* dq_statsp;
	char* readbuffp;
	size_t readbuffsize;
	size_t readitems;
	char deque_name[MAX_DEQUE_NAME_LEN];
	char filepath[PATH_MAX];
	int nitems;
	int itemsize;
	MMA_HANDLE* mmahp;
	FILE* f;
	size_t bytesread = 0;
	int items_pushed = 0;
	int mode = 0;				// 0 means ascii/raw transfer mode, 1 means binary
	unsigned int binarySequence = 0;		// binary mode sequence number
	int binaryHeaderSize = sizeof(unsigned int) + sizeof(size_t);
	
	if (cmdarg_fetch_switch(NULL, "i")) {
		fetch_values(deque_name, filepath, &nitems, &itemsize);
		mmahp = mmdq_open(deque_name);
		if (NULL == mmahp) {
			mma_strerror(ebuff, sizeof(ebuff));
			APP_ERR(stderr, ebuff);
		}
		
		// Open the file
		if (strcmp(filepath, "stdio") == 0) {
			f = stdin;
		} else {
			f = fopen(filepath, "r");
		}
		
		if (NULL == f) {
			APP_ERR(stderr, "Input File %s not found!\n", filepath);
		}
		dq_statsp = mmdq_stats(mmahp, &dq_stats);

		if (cmdarg_fetch_switch(NULL,"b")) {
			mode = 1;			// binary transfer mode
			if (dq_statsp->dqitem_size < binaryHeaderSize) {
				APP_ERR(stderr, "Deque item size too small for binary transfer must be at least %d", binaryHeaderSize+1);
			}
		}
		readbuffsize = 256;
		if (dq_statsp->dqitem_size > readbuffsize) {
			readbuffsize = dq_statsp->dqitem_size;
			readitems = 1;
		} else {
			readitems = readbuffsize / dq_statsp->dqitem_size;
			if (readbuffsize % dq_statsp->dqitem_size) {
				readitems++;
				readbuffsize = readitems * dq_statsp->dqitem_size;
			}
		}
		readbuffp = (char*)calloc(readitems, dq_statsp->dqitem_size);

		// Read up to readbuffsize characters
		bytesread = fread(readbuffp, 1, readbuffsize, f);
		while (bytesread > 0) {
			int bytesremaining;
			char* readp;
			
			bytesremaining = bytesread;
			readp = readbuffp;
			while (bytesremaining > 0) {
				if (mode == 1) {
					// Binary mode transfer. A header consisting of
					// 1) a sequence number and 2) valid data byte count
					// is written at the start of each deque item. This
					// will be followed by the data.
					binarySequence++;
					char* tbuffp = calloc(1, dq_statsp->dqitem_size);
					char* tbp = tbuffp;
					size_t bytesToCopy = 0;
					size_t itemDataBytes = dq_statsp->dqitem_size - binaryHeaderSize;
					bytesToCopy = (itemDataBytes < bytesremaining) ? itemDataBytes : bytesremaining;
					memcpy(tbp, &binarySequence, sizeof(binarySequence));
					tbp += sizeof(binarySequence);
					memcpy(tbp, &bytesToCopy, sizeof(size_t));
					tbp += sizeof(size_t);
					memcpy(tbp, readp, bytesToCopy);
					readp += bytesToCopy;
					if (mmdq_abd(mmahp, tbuffp)) {
						APP_ERR(stderr, "dequetool push encountered Deque Overflow after %d items\n", items_pushed);
					}
					bytesremaining -= bytesToCopy;
				} else {
					if (mmdq_abd(mmahp, readp)) {
						APP_ERR(stderr, "dequetool push encountered Deque Overflow after %d items\n", items_pushed);
					}
					readp += dq_statsp->dqitem_size;
					bytesremaining -= dq_statsp->dqitem_size;
				}
				items_pushed++;
			}
			memset(readbuffp, 0, readbuffsize);
			bytesread = fread(readbuffp, 1, readbuffsize, f);
		}
		if (!feof(f)) {
			APP_ERR(stderr, "Error reading Input File %s !\n", filepath);
		}
		free(readbuffp);
		fprintf(stderr, "dequetool push complete: %d items pushed\n", items_pushed); 
		return 1;		// action taken ... stop processing arguments
	}
	return 0; 			// no action taken ... keep looping
}

static int process_switch_e() {
	DQSTATS dq_stats;
	DQSTATS* dq_statsp;
	char* buffp;
	char deque_name[MAX_DEQUE_NAME_LEN];
	char filepath[PATH_MAX];
	int nitems;
	int itemsize;
	MMA_HANDLE* mmahp;
	FILE* f;
	size_t witems = 0;
	int items_popped = 0;
	int mode = 0; // ascii/raw mode = 0, 1 = binary
	unsigned long binarySequence = 1;
	int binaryHeaderSize = sizeof(unsigned int) + sizeof(size_t);
	
	if (cmdarg_fetch_switch(NULL, "e")) {
		fetch_values(deque_name, filepath, &nitems, &itemsize);
		mmahp = mmdq_open(deque_name);
		
		if (NULL == mmahp) {
			mma_strerror(ebuff, sizeof(ebuff));
			APP_ERR(stderr, ebuff);
		}
		// Open the file
		if (strcmp(filepath, "stdio") == 0) {
			f = stdout;
		} else {
			f = fopen(filepath, "w");
		}
		if (NULL == f) {
			APP_ERR(stderr, "Input File %s not found!\n", filepath);
		}
		dq_statsp = mmdq_stats(mmahp, &dq_stats);
		if (cmdarg_fetch_switch(NULL,"b")) {
			mode = 1;			// binary transfer mode
			if (dq_statsp->dqitem_size < binaryHeaderSize) {
				APP_ERR(stderr, "Deque item size too small for binary transfer must be at least %d", binaryHeaderSize+1);
			}
		}
		buffp = (char*)calloc(1, (dq_statsp->dqitem_size)); 
		while (!mmdq_rtd(mmahp, buffp)) {
			items_popped++;
			if (mode == 1) {
				// Binary mode ... get the sequence number
				unsigned int itemSeqNo;
				size_t dataSize;
				char* p = buffp;

				memcpy(&itemSeqNo, p, sizeof(itemSeqNo));	// get the sequence number
				if (itemSeqNo != binarySequence) {
					APP_ERR(stderr, "Binary transfer out of sequence as sequence number %d", binarySequence);
				}
				p += sizeof(itemSeqNo);
				memcpy(&dataSize, p, sizeof(size_t));		// get the data size
				p += sizeof(size_t);
				witems = fwrite(p, dataSize, 1, f);
				binarySequence++;		// increment expected sequence number
			} else {
				witems = fwrite(buffp, dq_statsp->dqitem_size, 1, f);
			}
			if (witems != 1) {
				APP_ERR(stderr, "Error writing output!");
			}
		}
		free(buffp);
		fprintf(stderr,"dequetool empty complete: %d items popped from deque\n", items_popped);
		return 1;		// action taken ... stop processing arguments
	}
	return 0; 			// no action taken ... keep looping
}

int main(int argc, char* argv[]) {

	// Switches are listed in order of processing precedence.

	static int switches[] = {'h', 'd', 'c', 'r', 'z', 'i', 'e', '\0'};
	static int (*process_func[])() = { 
		process_switch_help, 
		process_switch_d,
		process_switch_c,
		process_switch_r,
		process_switch_z,
		process_switch_i,
		process_switch_e,
		NULL
	};
	int status = 0;
	int finalstatus = 0;
	int i;
	
	register_args(argc, argv);
	
	status = cmdarg_parse(argc, argv);
	if (status) {
		cmdarg_show_help(NULL);
		exit(1);
	}
	appenv_register_env_var(MMDQ_DIR_PATH, NULL);
	
	// Parsed arguments successfully ... process. Handler routines
	// return > 0 if successful, 0 if they took no action, and < 0 
	// if they encountered an error.
	for (status = 0, i = 0; (status >= 0 && switches[i] != 0); i++) {
		status = (*process_func[i])();
		if (status != 0) {
			finalstatus = status;
			break;
		}
	}
	// final status > 0 means success!
	if (finalstatus > 0) {
		finalstatus = 0;
	} else if (finalstatus == 0) {
		finalstatus = 1;
	}
	return finalstatus;
}
