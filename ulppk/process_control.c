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
 * process_control.c
 *
 *  Created on: Nov 6, 2009
 *      Author: R_Garvey
 */

/**
 * @file process_control.c
 *
 * @brief Process management support.
 *
 * Process control provides tools for managing the spawning
 * of child processes, reaping their status, making a process a daemon,
 * etc.
 *
 */
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <process_control.h>
#include <ulppk_log.h>

static void fcds_throttle_increment(FCDS* fcdsp, PROC_STATUS* proc_statusp);
static void fcds_throttle_decrement(FCDS* fcdsp);
static PROC_STATUS* search_procs(FCDS* fcds, pid_t pid);
static PROC_STATUS* spawn_proc(FCDS* fcdsp, PROC_STATUS* proc_statusp);
static void scan_timeout_procs(FCDS* fcdsp);

/**
 * @brief Construct a new FCDS. If fcdsp is not NULL, it points to a buffer of at
 * least sizeof(FCDS) bytes that can be used to store the FCDS information.
 * Otherwise, memory is allocated from the heap (and must eventually be
 * returned by free). In either event, returns a pointer to the freshly
 * initialized FCDS structure.
 *
 * @param fcdsp Pointer to Flow Control Data Structure
 * @param proc_limit Max number of active child processes allowed.
 * @param throttle_exit Active process count must fall below this limit to exit flow control.
 * @param options Intended to be waitpid options, but not used.
 * @return Pointer to newly formatted flow control data structure. If fdcsp was NULL, then
 * 	this pointer points to memory allocated from the heap and must be released by calling free
 * 	at some point.
 */
FCDS* proc_new_fcds(FCDS* fcdsp, int proc_limit, int throttle_exit, int options) {

	if (NULL == fcdsp) {
		fcdsp = (FCDS*)calloc(1, sizeof(FCDS));
	}
	memset(fcdsp, 0, sizeof(FCDS));
	fcdsp->proc_limit = proc_limit;
	fcdsp->throttle_exit = throttle_exit;
	fcdsp->options = options;
	dq_init(proc_limit, sizeof(PROC_STATUS), &fcdsp->procs);
	return fcdsp;
}

/**
 * @brief Returns true if the FCDS is in a throttle condition.
 * @param fcdsp Pointer to Flow Control Data Structure
 * @return currrent state of throttle flag.
 */
int proc_in_throttle(FCDS* fcdsp) {
	return fcdsp->throttle_flag;
}

/**
 * @brief Perform a fork operation if not in throttle.
 *
 * This will spawn a "managed child process". Unmanaged child processes
 * can be forked by other mechanisms. (Note that proc_throttled_wait may reap the
 * exit status of unmanaged child processes. It will however
 * essentially ignore those statuses. This can present a risk to
 * designs that carelessly mingle managed and umanaged child processes,
 * and which require the exit status of unmanaged child processes.)
 *
 * Once a child process has been created, its pid is used to fill
 * out a PROC_STATUS structure, which is returned to the parent.
 *
 * Parent Process:
 *
 * The PROC_STATUS structure may be provided by the caller. Alternatively,
 * if proc_statusp is NULL memory for the PROC_STATUS is allocated by
 * using proc_new_proc_status. The calling parent process must subsequently
 * release this memory resource by calling proc_free_proc_status. In either
 * case, the function returns a pointer to the the PROC_STATUS structure.
 *
 * If the FCDS is in a throttle condition when this function is called,
 * it will return to the parent with a NULL PROC_STATUS pointer to indicate
 * no process was forked. The parent should then call proc_throttled_wait
 * to allow the exit status of terminated child processes to be reaped.
 *
 * If a fork error ocurrs, ULPPK_CRASH is invoked to produce a core file and a fatal
 * error in the log file.
 *
 * Child Process:
 *
 * The child process will receive a NULL return value from this process.
 *
 * @param fcdsp  ptr to FCDS structure
 * @param proc_statusp  NULL or pointer to a PROC_STATUS structure.
 * @returns  Pointer to PROC_STATUS structure. If returning to parent, PROC_STATUS
 *  pid is process ID of the forked child or -1 if the system in in throttle and no
 *  child process was spawned. If returning to child, PROC_STATUS field pid is 0.
 *
 */
PROC_STATUS* proc_throttled_fork(FCDS* fcdsp, PROC_STATUS* proc_statusp) {
	PROC_STATUS* oldprocp;
	PROC_STATUS waitcriteria;
	int status_allocated = 0;

	if (NULL == proc_statusp) {
		proc_statusp = proc_new_proc_status();
		status_allocated = 1;
	}

	if (proc_in_throttle(fcdsp)) {
		proc_statusp->pid = -1;
	} else {
		proc_statusp = spawn_proc(fcdsp, proc_statusp);
	}
	return proc_statusp;
}

/**
 * @brief Wait for a managed child process to terminate, and properly update the FCDS when it does.
 * This function treats managed children (spawned by calling proc_throttled_fork) and unmanaged
 * child (forked through any other means) differently. Termination of unmanaged child processes
 * is essentially ignored.
 *
 * pid and status of the terminated managed child process are stored in proc_statusp. If proc_statusp
 * provided by the caller is NULL, then proc_new_proc_status is used to allocate space to
 * store the process status info. The caller is responsible for releasing such memory by
 * calling proc_free_proc_status.
 *
 * This function essentially calls waitpid with the option provided. To provide a specific
 * pid value to waitpid, the caller must provide a non-null value for proc_statusp, set the
 * pid field of that structure to the appropriate value,  and set wait4pid to a nonzero value.
 *
 * The meaning of proc_statusp->pid if wait4pid is non-zero (as defined by waitpid)
 *
 *<ul>
 * <li>pid == -1  wait for any child process</li>
 * <li>pid > 0  wait for any child whose process ID equals pid</li>
 * <li>pid == 0  wait for any child whose process group ID equals that of the calling process</li>
 * <li>pid < -1  wait for any child whose process group ID equals the absolute value of pid</li>
 * </ul>
 *
 * See man waitpid for more details regarding option, exit statuses, etc.
 *
 * @param fcdsp  ptr to FCDS structure
 * @param proc_statusp  NULL or pointer to a PROC_STATUS structure. If not NULL and
 * 		wait4pid is non-zero, the pid field of the structure is used to call waitpid.
 * @param options  option to pass to waitpid
 * @param wait4pid  ignored if proc_statusp is NULL. If proc_statusp is not NULL and
 * 		wait4pid is non-zero, the the pid field of proc_statusp is passed to waitpid
 * @return  ptr to PROC_STATUS structure. If WNOHANG is set in options, and the return PROC_STATUS
 * 		structure pid = 0, then no child process meeting the criteria have terminated since the
 * 		last call. If pid = -1, an error occurred.
 *
 */
PROC_STATUS* proc_throttled_wait(FCDS* fcdsp, PROC_STATUS* proc_statusp, int options, int wait4pid){
	pid_t pid;
	pid_t pid2waitfor = -1;
	PROC_STATUS* list_proc_statusp;
	int procstat_allocated = 0;

	if (NULL == proc_statusp) {
		proc_statusp = proc_new_proc_status();
		procstat_allocated = 1;
	} else {
		if (wait4pid) {
			pid2waitfor = proc_statusp->pid;
		}
	}

	pid = waitpid(pid2waitfor, &proc_statusp->status, options);
	if (pid <= 0) {
		// Either an error or WNOHANG set and no child terminated
		// since last call to waitpid
		proc_statusp->pid = pid;
	}
	while (pid > 0) {
		// A child has terminated. It may be a managed child
		// or an unmanaged child. Find the corresponding PROC_STATUS
		// record in the FCDS list, if one exists. If no such status
		// record is found, then we are dealing with an unmanaged child.
		list_proc_statusp = search_procs(fcdsp, pid);
		if (NULL != list_proc_statusp) {
			// An managed child process terminated ... we have to manage throttle state
			// Decrement the process count under throttle rules. Then copy necessary
			// fields to proc_statusp and free list_proc_statusp.
			fcds_throttle_decrement(fcdsp);
			proc_statusp->pid = pid;
			proc_statusp->start_t = list_proc_statusp->start_t;
			proc_statusp->timeout_sec = list_proc_statusp->timeout_sec;
			proc_statusp->datap = list_proc_statusp->datap;
			proc_free_proc_status(list_proc_statusp);
			pid = 0;			// to force loop termination
		} else {
			// Non managed child process terminated. If options set WNOHANG, we
			// can return with a 0 pid. Otherwise, we should wait again. This
			// one of those cases where use of a goto might actually provide
			// superior program structure ...
			if (!(options & WNOHANG)) {
				pid = waitpid(pid2waitfor, &proc_statusp->status, options);
				if (-1 == pid) {
					ULPPK_LOG(ULPPK_LOG_ERROR, "-1 returned from waitpid unexepectedly!");
					if (procstat_allocated) {
						free(proc_statusp);
					}
					proc_statusp = NULL;
				}
			} else {
				pid = 0;
				proc_statusp->pid = 0;
			}
		}
	}
	return proc_statusp;
}

/**
 * @brief If you MUST allocate proc status from the heap, use this function and
 * return it with proc_free_proc_status
 *
 * @return Pointer to freshly allocated PROC_STATUS structure.
 */
PROC_STATUS* proc_new_proc_status() {
	PROC_STATUS* proc_statusp;

	proc_statusp = (PROC_STATUS*)calloc(1, sizeof(PROC_STATUS));
	proc_statusp->key = PROC_STATUS_GUARD1;
	return proc_statusp;
}

/**
 * @brief If you allocated a PROC_STATUS using proc_new_proc_status, you
 * MUST free it using proc_free_proc_status when done.
 *
 * @param proc_statusp Pointer to PROC_STATUS structure previously
 * 	allocated by proc_new_proc_status
 */
void proc_free_proc_status(PROC_STATUS* proc_statusp) {
	if (PROC_STATUS_GUARD1 == proc_statusp->key) {
		free(proc_statusp);
	} else {
		ULPPK_LOG(ULPPK_LOG_WARN, "Attempt to free PROC_STATUS not allocated from heap by proc_new_proc_status");
	}
}

/**
 * Make the calling process a daemon process. As a daemon process
 * 1) The calling process will be terminated.
 * 2) Execution proceeds under a new process that
 * 	a) is a session leader of a new session
 * 	b) is a procress group leader of a new process group
 * 	c) has no controlling terminal.
 *
 * @param workingdir  new working directory for the daemon. If NULL, then "/"
 * @param prohibit_ctty  if true, additional steps are taken to insure that the
 * 		newly established session can leader can never acquire a controlling
 * 		terminal. In that event, the daemon ceases to be a session leader.
 * @return 0 on success.
 */
int proc_make_daemon(char* workingdir, int prohibit_ctty) {
	pid_t pid;
	char* wdir;
	struct stat statbuf;
	int status;
	int ok2chdir = 1;
	int chdir_result = 0;

	if ((pid = fork()) < 0) {
		ULPPK_LOG(ULPPK_LOG_ERROR, "Fork error (make daemon)");
		return -1;		// fork error
	} else if (pid != 0) {
		exit(0);		// parent (1) immediately exits
	}
	if (prohibit_ctty) {
		// Fork again and cause parent (2) to exit
		if ((pid = fork()) < 0) {
			ULPPK_LOG(ULPPK_LOG_ERROR, "Fork error (make daemon prohibiting ctty");
			exit(1);	// gotta bail
		} else if (pid == 0) {
			// This is parent(2). Exit
			exit(0);
		}
	}

	// Continue as daemon. If prohibit_ctty is false, then we are a
	// session leader of a new session.
	if (NULL == workingdir) {
		chdir_result = chdir("/");
	} else {
		status = stat(workingdir, &statbuf);
		if (status) {
			ULPPK_LOG(ULPPK_LOG_ERROR, "Attempting to stat working dir %s", workingdir);
			ULPPK_LOG(ULPPK_LOG_ERROR, "Forcing working dir to /");
			chdir_result = chdir("/");
			ok2chdir = 0;
		}
		if (!S_ISDIR(statbuf.st_mode)) {
			ULPPK_LOG(ULPPK_LOG_ERROR, "Specified working dir is not a directory %s", workingdir);
			ULPPK_LOG(ULPPK_LOG_ERROR, "Forcing working dir to /");
			chdir_result = chdir("/");
			ok2chdir = 0;
		}
		if (ok2chdir){
			chdir_result = chdir(workingdir);
		}
	}
	umask(0);
	return 0;
}
/*
 * ****************************************
 * Static Functions
 * ****************************************
 */

static PROC_STATUS* spawn_proc(FCDS* fcdsp, PROC_STATUS* proc_statusp) {
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		ULPPK_CRASH("Fork error!");
	} else if (pid == 0) {
		// This is the child. Return a NULL PROC_STATUS pointer
		proc_statusp->pid = pid;
	} else {
		// This is the parent.
		proc_statusp->pid = pid;
		proc_statusp->status = 0;
		time(&proc_statusp->start_t);

		// Perform throttle incremenet. This will add
		// a copy of the PROC_STATUS structure to the
		// list of managed processes.
		fcds_throttle_increment(fcdsp, proc_statusp);
	}
	return proc_statusp;
}
static void fcds_throttle_decrement(FCDS* fcdsp) {
	if (fcdsp->proc_count > 0) {
		fcdsp->proc_count--;
	} else {
		ULPPK_LOG(ULPPK_LOG_DEBUG, "FCDS process count at zero and throttle decrement invoked");
	}
	if (fcdsp->proc_count < fcdsp->throttle_exit) {
		if (fcdsp->throttle_flag) {
			fcdsp->throttle_flag = 0;
			ULPPK_LOG(ULPPK_LOG_DEBUG, "Throttle Exit");
		}
	}
}
static void fcds_throttle_increment(FCDS* fcdsp, PROC_STATUS* proc_statusp) {
	if (fcdsp->proc_count < fcdsp->proc_limit) {
		// Add the proc status by incrementing proc_count and
		// pushing onto the list
		fcdsp->proc_count++;
		dq_abd(&fcdsp->procs, proc_statusp);
		proc_dump_proc_status(NULL, proc_statusp);
	}

	if (fcdsp->proc_count >= fcdsp->proc_limit) {
		if (!fcdsp->throttle_flag) {
			ULPPK_LOG(ULPPK_LOG_WARN, "Throttle Entry at proc_count %d", fcdsp->proc_count);
			fcdsp->throttle_flag = 1;
		}
	}
}
static PROC_STATUS* search_procs(FCDS* fcdsp, pid_t pid) {
	DQSTATS dqstats;
	DQSTATS* dqstatsp;
	PROC_STATUS list_proc_status;
	PROC_STATUS* outproc_statusp = NULL;
	DQHEADER* proc_listp;
	int i;
	int save_key;

	proc_listp = &fcdsp->procs;
	dqstatsp = dq_stats(proc_listp, &dqstats);
	if (dqstatsp->dquse != fcdsp->proc_count) {
		ULPPK_LOG(ULPPK_LOG_WARN, "FCDS proc list size != FCDS proc count: VERY UNEXPECTED");
	}
	for (i = 0; i < dqstatsp->dquse; i++) {
		memset(&list_proc_status, 0, sizeof(PROC_STATUS));
		if (!dq_rtd(proc_listp, &list_proc_status)) {
			// Successful pop from top of deque
			proc_dump_proc_status(NULL, &list_proc_status);
			if (list_proc_status.pid == pid) {
				// Found a matching record. We will copy its contents
				// to a new structure and let it fall of the deque.
				outproc_statusp = proc_new_proc_status();
				// Now we copy everything from list_proc_status to outproc_statusp EXCEPT
				// for the key field. outproc_statusp->key must be PROC_STATUS_GUARD1.
				save_key = outproc_statusp->key;
				memcpy(outproc_statusp, &list_proc_status, sizeof(PROC_STATUS));
				outproc_statusp->key = save_key;
				return outproc_statusp;
			} else {
				// Not our target process .. push back onto the bottom of the deque
				dq_abd(proc_listp, &list_proc_status);
			}
		} else {
			// this should never happen. If it does, log it and break out of here.
			ULPPK_LOG(ULPPK_LOG_ERROR, "FCDS list empty when it should have items ... error");
			break;
		}
	}
	return NULL;
}

/**
 * @brief Dump process status to the stream f. If f is NULL, log
 * to the ulppk log.
 *
 * @param f NULL means log via syslog, otherwise print to stream
 * @param proc_statusp  ptr to process status record to dump
 */
void proc_dump_proc_status(FILE* f, PROC_STATUS* proc_statusp) {
	if (f != NULL) {
		fprintf(f, "PROC_STATUS Dump:\n");
		fprintf(f,"key = 0x%X\n", proc_statusp->key);
		fprintf(f,"pid = %d\n", proc_statusp->pid);
		fprintf(f,"status = %d\n", proc_statusp->status);
		fprintf(f,"start_t = %d\n", (unsigned int)proc_statusp->start_t);
		fprintf(f,"timeout_sec = %d\n", (unsigned int)proc_statusp->timeout_sec);
		fprintf(f,"datap = %ld\n", (long)proc_statusp->datap);
	} else {
		ULPPK_LOG(ULPPK_LOG_DEBUG, "PROC_STATUS Dump:\n");
		ULPPK_LOG(ULPPK_LOG_DEBUG,"key = 0x%X\n", proc_statusp->key);
		ULPPK_LOG(ULPPK_LOG_DEBUG,"pid = %d\n", proc_statusp->pid);
		ULPPK_LOG(ULPPK_LOG_DEBUG,"status = %d\n", proc_statusp->status);
		ULPPK_LOG(ULPPK_LOG_DEBUG,"start_t = %d\n", proc_statusp->start_t);
		ULPPK_LOG(ULPPK_LOG_DEBUG,"timeout_sec = %d\n", proc_statusp->timeout_sec);
		ULPPK_LOG(ULPPK_LOG_DEBUG,"datap = %d\n", proc_statusp->datap);
	}
}
