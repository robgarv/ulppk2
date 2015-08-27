
/*
 *****************************************************************

<GPL>

Copyright: © 2001-2009 Robert C Garvey

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
 * @file mmatomx.c
 *
 * @brief Memory Mapped Atom Utility
 *
 * The mmatomx utility provides features for
 * <ul>
 * <li>Creating memory mapped atom files</li>
 * <li>Dumping memory mapped file contents</li>
 * </ul>
 *
 * The dump option is pretty minimalistic and not
 * very useful. Piping through od -c helps, but
 * a proper octal dump utility needs to be incorporated.
 *
 * mmmatomx -f /var/data/myfile.bin -d | od -c
 *
 * <h2>Command Line Use</h2>
 *
 * mmatomx -f [fullpath] -[dc]
 *
 * <ul>
 * <li>f -- full path to file (required) </li>
 * <li>d -- perform dump</li>
 * <li>c -- create file specified by -f option</li>
 * <li></li>
 * </ul>
 *
 * If the -c option is given, then the following additional options are available.
 * <ul>
 * <li>l -- Length of file/mem mapped region in bytes (default 4096)</li>
 * <li>t -- Logical tag for memory mapped atom (default MyObject)</li>
 * <li>m -- Protection mode Protection mode READ_ONLY, WRITE_ONLY, READ_WRITE (default READ_WRITE)</li>
 * <li>M -- Map Flags: shared or private (default shared)</li>
 * <li>p -- File permissions/access mode (octal) (default 664)</li>
 * </ul>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <cmdargs.h>
#include <mmapfile.h>

// Function prototypes
static void register_args(int argc, char* argv[]);
static int process_switch_create();
static int process_switch_dump();

// Main program

int main(int argc, char* argv[]) {
	int status;
	
	register_args(argc, argv);

	status = cmdarg_parse(argc, argv);
	if (status) {
		cmdarg_show_help(NULL);
		exit(1);
	}
	
	// Parsed arguments successfully ... process. Handler routines
	// return > 0 if successful, 0 if they took no action, and < 0 
	// if they encountered an error.
	
	status = process_switch_create();
	if (0 == status) {
		status = process_switch_dump();
	}
	if (status > 0) {
		status = 0;
	}
	return status;

}

// Register command line arguments with the cmdargs library.

static void register_args(int argc, char* argv[]) {
	cmdarg_init(argc, argv);
	// Create function
	cmdarg_register_option("c", "create", CA_SWITCH,
		"Create Memory Mapped Atom", NULL, NULL);
		
		cmdarg_register_option("l", "length", CA_DEFAULT_ARG,
			"Length of file/mem mapped region in bytes", "4096", "c");
		cmdarg_register_option("t", "tag", CA_DEFAULT_ARG, 
			"Logical tag for memory mapped atom", "MyObject", "c");
		cmdarg_register_option("m", "mode", CA_DEFAULT_ARG,
			"Protection mode READ_ONLY, WRITE_ONLY, READ_WRITE", "READ_WRITE", "c");
		cmdarg_register_option("M", "mapflags", CA_DEFAULT_ARG, 
			"Map Flags: shared or private", "shared", "c");
		cmdarg_register_option("p", "permissions", CA_DEFAULT_ARG,
			"File permissions/access mode (octal)", "664", "c");
		
	// Dump function
	
	cmdarg_register_option("d", "dump", CA_SWITCH,
		"Dump contents of a file", NULL, NULL);
		
	// Filepath required argument
	cmdarg_register_option("f", "filepath", CA_REQUIRED_ARG, 
		"Full path of disk file for memory mapped I/O", NULL, "c");
 		
}
static void app_error(char* errmsg) {
	if ((errmsg != NULL) && (strlen(errmsg) != 0)) {
		char errbuff[2048];
		sprintf(errbuff, "Application Error: Diagnostic message follows:\n %s \n",
			errmsg);
		fprintf(stderr, "%s", errbuff);
	} else {
		fprintf(stderr,"Application Error -- No diagnostic message available\n");
	} 
	exit(1);
}

static int process_switch_dump() {
	static char errbuff[128];
	int status = 0;
	char* strfilename;
	MMA_HANDLE* mmahp;
	
	if (cmdarg_fetch_switch(NULL, "d")) {
		status = 1;
		strfilename = cmdarg_fetch_string(NULL, "f");
		if (NULL == strfilename) {
			sprintf(errbuff, "Filename not provided! Aborting\n");
			app_error(errbuff);
		}
		mmahp = mmapfile_open("object1", strfilename, MMA_READ, MMF_SHARED);
		if (NULL == mmahp) {
			char txtbuff[1024];
			sprintf(errbuff, "Error mapping file %s\n%s\n", strfilename, 
				mma_strerror(txtbuff, sizeof(txtbuff)));
			app_error(errbuff);
		}
		// Print some statistics
		fprintf(stdout, "File: %s\n", mmahp->u.df_refp->str_pathname);
		fprintf(stdout, "Map Size: %ld\n", mmahp->mm_ref.len);
		fprintf(stdout, "File Size: %ld\n", mmahp->u.df_refp->len);
		fprintf(stdout, "====================================\n");
		// For right now, just do an ascii print
		fprintf(stdout, "%s", (char*)mma_data_pointer(mmahp));
		fprintf(stdout, "\n===========DUMP COMPLETE=============\n");
	}
	return status;
}

static MMA_MAP_FLAGS validate_map_flags(char* strflags) {
	MMA_MAP_FLAGS flags;
	if (!strcasecmp("private", strflags)) {
		flags = MMF_PRIVATE;
	} else if (!strcasecmp("shared", strflags)) {
		flags = MMF_SHARED;
	} else {
		char errbuff[64];
		sprintf(errbuff,"Invalid MAP_FLAG [%s]\n"
			"Valid values are SHARED and PRIVATE\n", strflags);
		app_error(errbuff);
	}
	return flags;
}
static MMA_ACCESS_MODES validate_access_mode(char* strmode) {
	MMA_ACCESS_MODES mode;
	
	if (!strcasecmp("read_only",strmode)) {
		mode = MMA_READ;
	} else if (!strcasecmp("write_only", strmode)) {
		mode = MMA_WRITE;
	} else if (!strcasecmp("read_write", strmode)) {
		mode = MMA_READ_WRITE;
	} else {
		char errbuff[256];
		sprintf(errbuff,
			"Invalid Access Mode [%s]\n"
			"Valid modes are\n"
			"READ_ONLY, WRITE_ONLY, READ_WRITE\n",
			strmode);
		app_error(errbuff);
	}
	return mode;
}

#if 0
static int process_switch_create() {
	static char errbuff[1024];
	int status = 0;
	long length;
	char* strmapflags;
	char* strmode;
	char* strfilename;
	char* strtag;
	int fpermission;
	char* mmp;
	MMA_OBJECT_TYPES obj_type;
	MMA_ACCESS_MODES access_mode;
	MMA_MAP_FLAGS flags;
	MMA_HANDLE* mmahp;
	MMA_DISK_FILE_REF* refp;
	
	if (cmdarg_fetch_switch(NULL, "c")) {
		status = 1;				// we're gonna do something -- -c switch found
		length = cmdarg_fetch_long(NULL, "l");
		strmapflags = cmdarg_fetch_string(NULL, "M");
		strmode = cmdarg_fetch_string(NULL, "p");
		strtag = cmdarg_fetch_string(NULL, "t");
		flags = validate_map_flags(strmapflags);
		strfilename = cmdarg_fetch_string(NULL,"f");
		fpermission = cmdarg_fetch_int(NULL, "m");
		if (NULL == strfilename) {
			app_error("-f option not found!");
		}			
		access_mode = validate_access_mode(strmode);
		obj_type = MMT_FILE;
		
		// Create the disk file reference
		refp = mma_new_disk_file_ref(strfilename, (O_CREAT | O_RDWR | O_TRUNC),
			fpermission, length);
		if (NULL == refp) {
			fprintf(stderr, "Error creating Disk File Reference!\n");
			app_error(mma_strerror(errbuff, sizeof(errbuff)));
		}
		mmahp = mma_create(strtag, obj_type, access_mode, refp, length, flags);
		if (NULL == mmahp) {
			fprintf(stderr, "Error creating memory mapped object!\n");
			app_error(mma_strerror(errbuff, sizeof(errbuff)));
		}
		mmp = (char*)mmahp->mm_ref.pa;
		fprintf(stdout, "Memory mapped file created and initialized: %s --  %ld bytes\n", strfilename, length);
	}
	return status;	
}
#else
static int process_switch_create() {
	static char errbuff[1024];
	int status = 0;
	long length;
	char* strmapflags;
	char* strmode;
	char* strfilename;
	char* strtag;
	int fpermission;
	MMA_ACCESS_MODES access_mode;
	MMA_MAP_FLAGS flags;
	MMA_HANDLE* mmahp;
	
	if (cmdarg_fetch_switch(NULL, "c")) {
		status = 1;				// we're gonna do something -- -c switch found
		length = cmdarg_fetch_long(NULL, "l");
		strmapflags = cmdarg_fetch_string(NULL, "M");
		strmode = cmdarg_fetch_string(NULL, "p");
		strtag = cmdarg_fetch_string(NULL, "t");
		flags = validate_map_flags(strmapflags);
		strfilename = cmdarg_fetch_string(NULL,"f");
		fpermission = cmdarg_fetch_int(NULL, "m");
		if (NULL == strfilename) {
			app_error("-f option not found!");
		}			
		access_mode = validate_access_mode(strmode);
		mmahp = mmapfile_create("object1", strfilename, length,
			access_mode, flags, fpermission);
		if (mmahp == NULL) {
			sprintf(errbuff, "Attempt to create memory mapped file %s fails!\n",
				strfilename);
			app_error(errbuff);
		}
		fprintf(stderr, "Created Memory Mapped File: %s Size %ld\n", 
			strfilename, length);
	}
	return status;
}
#endif
