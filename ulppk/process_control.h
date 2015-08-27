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

/*
 * process_control.h
 *
 *  Created on: Nov 6, 2009
 *      Author: R_Garvey
 */

#ifndef PROCESS_CONTROL_H_
#define PROCESS_CONTROL_H_

/**
 * @file process_control.h
 *
 * @brief Definitions for the Process Control module
 *
 * Process control provides tools for managing the spawning
 * of child processes, reaping their status, making a process a daemon,
 * etc.
 */
#include <stdio.h>
#include <dqacc.h>
/**
 * Fork control data structure. Used by limited (throttled) process management facilities.
 * This provides a flow control or throttle mechanism. Throttle is posted when an attempt
 * to fork a process would exceed the established process limit. Throttle remains in effect
 * until the number of active processes falls below the throttle exit limit.
 */
typedef unsigned int FCDS_OPTION;

/**
 * FCDS_BLOCK ... if throttle, loop until throttle exit is achieved
 * FCDS_TIMEOUT ... monitor running processes and abort those who have exceed their timeouts
 *   Ignored if FCDS_BLOCK is not also specified.
 */
#define FCDS_BLOCK  	0x0001
#define FCDS_TIMEOUT	0x0002

/**
 * Flow Control Data Structure. Used to manage "throttling" of the
 * spawning of child processes.
 */
typedef struct {
	int proc_limit;			///< max number of processes allowed.
	int throttle_exit;		///< throttle exit limit
	int proc_count;			///< current number of processes running
	int throttle_flag;		///< true if in throttle
	FCDS_OPTION options;	///< flow control option
	DQHEADER procs;			///< A list of process status structures
} FCDS;

#define PROC_STATUS_GUARD1 0xA55AA55A

/**
 * Process status structures are used to manage processes
 * and reap their exit status.
 */
typedef struct {
	int key;          		///< if 0xA55AA55A assume from heap
	pid_t pid;         		///< process ID
	int status;        		///< process status
	time_t start_t;    		///< time of process startup
	time_t timeout_sec;		///< seconds to allow process to run unfettered.
	void* datap;       		///< pointer to application specific data
} PROC_STATUS;

#ifdef __cplusplus
extern "C" {
#endif

	void proc_dump_proc_status(FILE* f, PROC_STATUS* proc_statusp);
	void proc_free_proc_status(PROC_STATUS* proc_statusp);
	int proc_in_throttle(FCDS* fcdsp);
	int proc_make_daemon(char* workingdir, int prohibit_ctty);
	FCDS* proc_new_fcds(FCDS* fcdsp, int proc_limit, int throttle_exit, int options);
	PROC_STATUS* proc_new_proc_status();
	PROC_STATUS* proc_throttled_fork(FCDS* fcdsp, PROC_STATUS* proc_statusp);
	PROC_STATUS* proc_throttled_wait(FCDS* fcdsp, PROC_STATUS* proc_statusp, int options, int wait4pid);

#ifdef __cplusplus
}
#endif

#endif /* PROCESS_CONTROL_H_ */
