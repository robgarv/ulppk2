
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
 * @file mmbuffpool.c
 *
 * @brief Memory mapped buffer pool maintenance utility.
 *
 * mmbuffpool provides features for creating memory mapped buffer pools, resetting to
 * inital state, displaying pool deque contents and reporting on their current state.
 *
 * Command line options.
 *
 * <ul>
 * <li>-p --poolname : Buffer pool name</li>
 * <li>-d --pooldir : Pool data directory overrides environment setting
 * <li>-c --create : Create buffer pool</li>
 * <li>-l --datalength : Length of user available data of buffer in bytes (create only) </li>
 * <li>-i --poolid : Numeric pool identifier. An integer (default is 1)
 * <li>-C --capacity ; Required number of buffers in pool (create only)</li>
 * <li>-r --report : Report buffer pool stats</li>
 * <li>-D --display ; Display buffer pool deque contents (buffer indices) </li>
 * <li>-z --zap : Reset buffer pool to initialized state</li>
 * </ul>
 *
 * Environment Variables:
 *
 * The buffer pool management code (mmpool.c) refers to the environment variable
 * MMPOOL_DATA_DIR. The user of this program can override this variable by
 * providing the -P or --pooldir option.
 *
 * Examples:
 *
 * Assume MMPOOL_DATA_DIR=/var/ulppk/memfiles and that the directory exists and the
 * user invoking the utility has proper rwx access to the directory. Assume the user
 * name is ulppkuser. (It could be anything.)
 *
 * To create a buffer pool consisting of 64 buffers each 4 K long:
 *
 * mmbuffpool -c -p pool4k -l 4096 -C 64
 *
 * This will create following files in the /var/ulppk/memfiles directory
 * <pre>
 * -rw-rwS--- 1 ulppkuser ulppkuser 266240 Jan  6 15:30 pool64k.bpcf
 * -rw-rwS--- 1 ulppkuser ulppkuser   4096 Jan  6 15:30 pool64k.bpmf
 * </pre>
 *
 * To create an identical pool in the /home/ulppkuser directory:
 *
 * mmbuffpool -c -p pool4k -l 4096 -C 64 -d /home/ulppkuser
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>

#include <cmdargs.h>
#include <mmpool.h>
#include <appenv.h>
#include <pathinfo.h>

// Function prototypes
static char* fetch_pool_dir(char* pool_dir, int dir_required);
static void app_error(char* err_msg);
static void register_args(int argc, char* argv[]);
static int process_switch_help();
static int process_switch_create();
static int process_switch_report();
static int process_switch_display();
static int process_switch_zap();
static int report_pool(BPOOL_HANDLE* bphp);
static int display_pool(BPOOL_HANDLE* bphp);
static void rptline(char* fmtp, ...);

static char err_buff[2048];


int main(int argc, char* argv[]) {
	int status;
	int require_pooldir = 0;
	char* pooldir = NULL;
	char* overridedir = NULL;
	
	// Register the MMPOOL_ENV_DATA_DIR variable
	// in the application environment
	pooldir = appenv_register_env_var(MMPOOL_ENV_DATA_DIR, "/var/ulppk/data");
	if (! pathinfo_is_dir(pooldir)) {
		// The path specified by environment variable MMPOOL_ENV_DATA_DIR
		// does exist. Either a) the variable is not defined in the calling
		// shell environment and the default value /var/ulppk/data is incorrect
		// or b) the environment variable was set incorrectly. CHECK PERMISSIONS
		// because pathinfo_is_dir can be deceived by a directory it does not
		// have permission to access. In any case, we now NEED the -d option
		// in the command line.
		require_pooldir = 1;
	}
	register_args(argc, argv);

	status = cmdarg_parse(argc, argv);
	if (status) {
		cmdarg_show_help(NULL);
		exit(1);
	}
	
	// Parsed arguments successfully ... process. First,
	// check is user is just asking for help. If so, program
	// will print help and exit.
	status = process_switch_help();

	// Then get the pool directory override (if any). Note fetch_pool_dir
	// can return the original environment pool dir is -d
	// is not provided or is incorrect.
	overridedir = fetch_pool_dir(pooldir, require_pooldir);
	appenv_set_env_var(MMPOOL_ENV_DATA_DIR, overridedir);

	// Handler routines
	// return > 0 if successful, 0 if they took no action, and < 0 
	// if they encountered an error.
	if (0 == status) {
		status = process_switch_create();
	}
	if (0 == status) {
		status = process_switch_report();
	}
	if (0 == status) {
		status = process_switch_display();
	}
	if (0 == status) {
		status = process_switch_zap();
	}
	if (status > 0) {
		status = 0;
	}
	return status;
}


// Register command line arguments with the cmdargs library.

static void register_args(int argc, char* argv[]) {
	cmdarg_init(argc, argv);
	
	// pool name parameter
	cmdarg_register_option("p", "poolname", CA_REQUIRED_ARG,
		"Buffer pool name", NULL, NULL);

	// Pool directory path override option
	cmdarg_register_option("d", "pooldir", CA_DEFAULT_ARG,
		"Buffer pool data directory override", "", NULL);

	// Create function

	cmdarg_register_option("h", "help", CA_SWITCH,
		"Create Memory Mapped Atom", NULL, NULL);

	cmdarg_register_option("c", "create", CA_SWITCH,
		"Create Memory Mapped Atom", NULL, NULL);
		
	cmdarg_register_option("l", "datalength", CA_DEFAULT_ARG,
		"Length of user data of buffer in bytes", "4", "c");
	cmdarg_register_option("C", "capacity", CA_DEFAULT_ARG,
		"Requested pool capacity", "20", "c");
	cmdarg_register_option("i", "poolid", CA_DEFAULT_ARG,
		"Pool ID number", "1", "c");
		
	// Report function
	
	cmdarg_register_option("r", "report", CA_SWITCH,
		"Report buffer pool stats", NULL, NULL);
	
	// Dump function
	cmdarg_register_option("D", "display", CA_SWITCH,
		"Display pool deque contents", NULL, NULL);
		
	// Zap function
	cmdarg_register_option("z", "zap", CA_SWITCH, 
		"Reset pool to initial state", NULL, "c");
 		
}

static char* fetch_pool_dir(char* pool_dir, int dir_required) {
	char* pool_dir_override;
	char* outpooldir;

	pool_dir_override = cmdarg_fetch_string(NULL, "d");

	if (dir_required) {
		if ((pool_dir_override == NULL ) || (strlen(pool_dir_override) == 0)) {
			// Command line did not give us a directory path for the pool data files
			// and neither did the environment ... can't go on
			app_error("Environment does not provide valid MMPOOL_ENV_DATA_DIR setting: -d option needed");
		} else {
			if (pathinfo_is_dir(pool_dir_override)) {
				outpooldir = pool_dir_override;
			} else {
				app_error("-d option did not provide accessible directory path");
			}
		}
	} else {
		// The environment gave us a good path but that can be overriden
		// if -d option provides a valid data path
		if ((pool_dir_override == NULL ) || (strlen(pool_dir_override) == 0)) {
			// Command line did not give us a directory path for the pool data files
			// but environment did ... use environment
			outpooldir = pool_dir;
		} else {
			if (pathinfo_is_dir(pool_dir_override)) {
				outpooldir = pool_dir_override;
			} else {
				outpooldir = pool_dir;
			}
		}
	}
	return outpooldir;
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

static int process_switch_help() {

	if (cmdarg_fetch_switch(NULL, "h")) {
		cmdarg_show_help(NULL);
		exit(0);
	}
	return 0;
}

static int process_switch_create() {
	int status = 0;
	char* pool_name;
	long data_size;
	long req_capacity;
	unsigned short pool_id = 0;
	CMD_ARG* optp;
	BPOOL_HANDLE* bphp;
	
	if (cmdarg_fetch_switch(NULL, "c")) {
		status = 1;
		optp = cmdarg_fetch(NULL, "c");
		data_size = cmdarg_fetch_long(optp, "l");
		req_capacity = cmdarg_fetch_long(optp, "C");
		pool_id = (unsigned short)cmdarg_fetch_int(optp, "i");
		pool_name = cmdarg_fetch_string(NULL, "p");
		bphp = mmpool_define_pool(pool_name, pool_id, data_size, req_capacity);
		if (bphp != NULL) {
			report_pool(bphp);
		} else {
			status = -1;
			app_error(mma_strerror(err_buff, sizeof(err_buff)));
		}
	}
	return status;
}

static int process_switch_report() {
	int status = 0;
	char* pool_name;
	BPOOL_HANDLE* bphp;
	
	if (cmdarg_fetch_switch(NULL, "r")) {
		pool_name = cmdarg_fetch_string(NULL, "p");
		bphp = mmpool_open(pool_name);
		if (bphp != NULL) {
			report_pool(bphp);
		} else {
			status = -1;
			app_error(mma_strerror(err_buff, sizeof(err_buff)));
		}
	}
	return 0;
}
static int process_switch_display() {
	int status = 0;
	char* pool_name;
	BPOOL_HANDLE* bphp;
	
	if (cmdarg_fetch_switch(NULL, "D")) {
		pool_name = cmdarg_fetch_string(NULL, "p");
		bphp = mmpool_open(pool_name);
		if (bphp != NULL) {
			// Report on pool stats
			report_pool(bphp);
			// Report on pool contents
			display_pool(bphp);
		} else {
			status = -1;
			app_error(mma_strerror(err_buff, sizeof(err_buff)));
		}
	}
	return 0;
}

static int process_switch_zap() {
	return 0;
}

static int report_pool(BPOOL_HANDLE* bphp) {
	rptline("=========== BUFFER POOL REPORT =================");
	rptline("");
	rptline("Name: %s ID: %d Capacity: %d Req-Capacity %d",
		bphp->bpmf_recp->stats.name,
		bphp->bpmf_recp->stats.bp_id,
		bphp->bpmf_recp->stats.capacity,
		bphp->bpmf_recp->stats.rqst_capacity
	);
	rptline("In the pool: In: %d Capacity: %d Pct: %g",
		bphp->bpmf_recp->dq_inpool.dquse,
		bphp->bpmf_recp->dq_inpool.dqslots,
		(100.0 * bphp->bpmf_recp->dq_inpool.dquse)/ bphp->bpmf_recp->dq_inpool.dqslots
	);
	rptline("Out of pool: In: %d Capacity: %d Pct: %g",
		bphp->bpmf_recp->dq_outpool.dquse,
		bphp->bpmf_recp->dq_outpool.dqslots,
		(100.0 * bphp->bpmf_recp->dq_outpool.dquse)/ bphp->bpmf_recp->dq_outpool.dqslots
	);
	rptline("Max Data Size: %ld", bphp->bpmf_recp->stats.max_data_size);
	return 0;
}

static void rptline(char* fmtp, ...) {
	va_list ap;
	
	va_start(ap, fmtp);
	vfprintf(stdout, fmtp, ap);
	fprintf(stdout,"\n");
}

static int display_pool(BPOOL_HANDLE* bphp) {
	DQHEADER* dqp;
	DQHEADER tempdq;
	int buffcount = 0;
	BPOOL_INDEX bpx = 0;
	
	// TODO: lock
	dqp = &bphp->bpmf_recp->dq_inpool;

	// Construct a temporary deque header
	dq_init(dqp->dqslots, dqp->dqitem_size, &tempdq);
		
	for (bpx = 0; (!dq_rtd(dqp,  &bpx)); ) {
		buffcount++;
		dq_abd(&tempdq, &bpx);			// push onto the temporary deque
		rptline("Rec: %d BPX: %ld", buffcount, bpx);
	}
	if (buffcount != bphp->bpmf_recp->stats.remaining) {
		rptline("WARNING: Header balance problem! Buffcount = %d stats.remaining = %d",
			buffcount, bphp->bpmf_recp->stats.remaining);
	}
	// Restore the memory mappe deque to its original state
	for (bpx = 0; (!dq_rtd(&tempdq, &bpx)); ) {
		dq_abd(dqp, &bpx);
	}
	// TODO: Unlock
	return buffcount;
}
