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
 * @file socketserver.c
 *
 * @brief A skeletal socket server framework.
 *
 * See socketserver.h for a overview of the socket server skeleton,
 * command line options it processes, and the life cycle of a socket
 * server.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "socketserver.h"
#include "diagnostics.h"
#include "signalkit.h"
#include "cmdargs.h"

static SSRVR_HANDLE* ssrvrp;
static int listenfd;
static int listen_port = 49152;
static int backlog = 5;
static void* data_ptr = NULL;
static int open_conns = 0;
static int max_open_conns = 64;
static int loop_abort = 0;
static int listenfd_forced_closed = 0;

// Functions to set up and start a socket server.

// This function constructs a new socket server handle structure.
// Personality functions are initialize to stubs.
// SSRVR_HANDLE* ssrvr_new();

struct sockaddr_in local_addr;

#if 0
static char* parse_int_arg(char* arg, long* valp) {
	char* msgp = NULL;
	char emsg[256];
	
	*valp = strtol(arg, NULL, 0);
	if ((*valp == 0) && (errno != 0)) {
		// For some reason in test, passing invalid data
		// to this function does not set errno.
		sprintf(emsg, "Error converting string %s to integer. errno = %d | %s",
			arg, errno, strerror(errno));
		msgp = emsg;
	}
	return msgp;
}

#endif

/**
 * The socket server must accept the following
 * command line arguments:
 *
 * -h -- command line help switch
 * -b (--backlog) -- accept queue backlog limit (default is 5)
 * -m (--maxconns) -- max open connections (default is 64)
 * -p (--port) -- listen port (default is 49152)
 *
 * This function should be called by the main program
 * before registering any application specific command line arguments!
 * If the calling program has no app specific command line arguments,
 * then it can just call this and start the shroud.
 */
static int ssrvr_cmdarg_init(int argc, char* argv[]) {
	int status = 0;

	cmdarg_init(argc, argv);

	status |= cmdarg_register_option("h", "help", CA_SWITCH, "Get help on this program", NULL, NULL);
	status |= cmdarg_register_option("p","port", CA_DEFAULT_ARG,
			"Hostname of target (default is 49152)", "49152", NULL);
	status |= cmdarg_register_option("b","backlog", CA_DEFAULT_ARG,
			"Accept queue backlog", "5", NULL);
	status |= cmdarg_register_option("m","maxconn", CA_DEFAULT_ARG,
			"Max open connections (default is 64)", "64", NULL);

	return status;
}

/*
 * This function is called by the shroud to parse command line arguments.
 * The caller of the shroud may defined additional arguments in its
 * own registration logic ... and may call the cmdargs_parse function
 * before calling the shroud.
 */
static int parse_clargs(int argc, char* argv[]) {
	int ix;
	char *cp;
	int val;
	int helpflag;

	if (cmdarg_parse(argc, argv)) {
		cmdarg_show_help(NULL);
		return 1;
	}

	// Check for help flag. If provided, show
	// command line help and exit.
	helpflag = cmdarg_fetch_switch(NULL, "h");
	if (helpflag) {
		cmdarg_show_help(NULL);
		exit(1);
	}
	listen_port = cmdarg_fetch_int(NULL, "p");
	backlog = cmdarg_fetch_int(NULL, "b");
	max_open_conns = cmdarg_fetch_int(NULL, "m");
	return 0;
}
	
//
// The default child signal handler. 

static void sig_child_handler(int signo) {

	pid_t pid;		// pid of the child
	int	stat;		// exit status of the child
	
	while ((pid =waitpid(-1, &stat, WNOHANG)) > 0) {
		open_conns--;
	}
	return;
}

// SIGTERM signal handler sets loop_abort flag and forces closure of
// the listen socket.
static void sig_term_handler(int signo) {
	loop_abort = 1;
	listenfd_forced_closed = 1;
	close(listenfd);
	return;
}

static int setup_listen_socket() {
	int result = 0;
	
	listenfd = socket(PF_INET, SOCK_STREAM, 0);
	        // Now bind the necessary port
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(listen_port);
	
	result = bind(listenfd, (struct sockaddr*)&local_addr, sizeof(struct sockaddr_in));
	if (result != 0 ) {
		char errmsg[256];

		ERR_MSG(errmsg, "bind error:");
		fprintf(stderr, "%s errno = %d | %s", errmsg, errno, strerror(errno));
		return -1;
	} 
	result = listen(listenfd, backlog);
	if (result != 0) {
		char errmsg[256];

		ERR_MSG(errmsg, "listen error:");
		fprintf(stderr, "%s errno = %d | %s", errmsg, errno, strerror(errno));
		return -1;
		
	}
	
	return listenfd;
}

/**
 * @brief This function constructs a new socket server handle structure.
 * Personality functions are initialize to stubs.
 * @return pointer to a socket server handle structure.
 */
SSRVR_HANDLE* ssrvr_new() {
	SSRVR_HANDLE* hp;
	
	hp = (SSRVR_HANDLE*)malloc(sizeof(SSRVR_HANDLE));
	memset(hp, 0, sizeof(SSRVR_HANDLE));
	return hp;
}

/**
 * @brief Register a Command Line Personality Function (CLPF)
 *
 * Once a handle is created, personality functions may be registered.
 *
 * If there are custom command line arguments to add (in addition
 * to the stock command line options ever socket server skeleton
 * must handle), then a CLPF should be implemented and registered
 * by calling this function. The CLPF should simply call the
 * cmdarg_register_option function to register these custom
 * arguments.
 *
 * @param pHandle Pointer to SSRVR_HANDLE returned by ssrvr_new
 * @param clpf Pointer to Command Line Personality Function
 *
 */
void ssrvr_register_clpf(SSRVR_HANDLE* pHandle, SSRVR_CLPF *clpf) {
	pHandle->clpf = clpf;
}

/**
 * @brief Register an Initialization Personality Function (IPF)
 *
 * The IPF is called by ssrvr_start after the CLPF (Command Line
 * Personality Function) has been called to register any special
 * command line options, and the command line options have been parsed.
 * The CLPF should register those options ONLY.
 * The IPF can read the option values.
 *
 * @param pHandle Pointer to SSRVR_HANDLE returned by ssrvr_new
 * @param ipf Pointer to Initialization Personality Function
 */
void ssrvr_register_ipf(SSRVR_HANDLE* pHandle, SSRVR_IPF *ipf) {
	pHandle->ipf = ipf;
}

/**
 * @brief Register a Main Personality Function (MPF)
 *
 * Once a handle is created, personality functions may be registered.
 *
 * The MPF is called by ssrvr_start after a socket connection has
 * been accepted. The MPF runs under a child process. The parent
 * process loops to accept a new connection.
 *
 * @param pHandle Pointer to SSRVR_HANDLE returned by ssrvr_new
 * @param mpf Pointer to Main Personality Function
 */
void ssrvr_register_mpf(SSRVR_HANDLE* pHandle, SSRVR_MPF *mpf) {
	pHandle->mpf = mpf;
}

/**
 * @brief Register a Child Termination Personality Function (CTPF)
 *
 * Once a handle is created, personality functions may be registered.
 *
 * The CTPF is called by ssrvr_start after the MPF has returned to
 * the child skeleton. A CTPF should be implemented to perform any
 * "must do" clean up after the MPF has completed processing.
 *
 * @param pHandle Pointer to SSRVR_HANDLE returned by ssrvr_new
 * @param ctpf Pointer to Child Termination Personality Function
 */
void ssrvr_register_ctpf(SSRVR_HANDLE* pHandle, SSRVR_CTPF *ctpf) {
	pHandle->ctpf = ctpf;
}

/**
 * @brief Register a Parent Termination Personality Function (PTPF)
 *
 * Once a handle is created, personality functions may be registered.
 *
 * The PTPF is called by ssrvr_start after main accept loop has
 * been terminated. Loop termination (loop abort) will occur when
 * a SIGTERM is sent to the parent. A PTPF should be implemented
 * to perform any"must do" clean up after the accept loop has
 * been aborted.
 *
 * @param pHandle Pointer to SSRVR_HANDLE returned by ssrvr_new
 * @param ptpf Pointer to Parent Termination Personality Function
 */
void ssrvr_register_ptpf(SSRVR_HANDLE* pHandle, SSRVR_PTPF *ptpf) {
	pHandle->ptpf = ptpf;
}

/**
 * @brief Start the socket server.
 *
 * Once the required personality functions have been registered, the
 * socket server can be started by calling this function. This function
 * parses command line arguments, drives the calls to the registered
 * personality functions, handles listen socket setup, and accepts
 * connections. To terminate the accept loop, send a SIGTERM to the
 * parent process.
 *
 * @param pHandle Pointer to SSRVR_HANDLE returned by ssrvr_new
 * @param datap Pointer to application data. All personality functions
 * 	will have access to this pointer.
 * @param argc Program argument count, as passed to the main() funtion
 * @param argv Array of program argument strings, as passed to the main() funciton.
 */
int ssrvr_start(SSRVR_HANDLE* pHandle, void* datap, int argc, char* argv[]) {
	int rstatus = 0;
	struct sockaddr_in clientaddr;
	int connfd = 0;
	int retstat = 0;
	pid_t childpid;
	
	if (pHandle == NULL) {
		fprintf(stderr, "NULL server handle not allowed! Aborting!\n");
		return 1;
	}
	ssrvrp = pHandle;			// set the server handle pointer
	data_ptr = datap;			// set the data pointer
	

	// Init command line arg processing and call the
	// Command Line Personality Function
	ssrvr_cmdarg_init(argc, argv);
	if (NULL != ssrvrp->clpf) {
		rstatus = (*ssrvrp->clpf)(argc, argv, data_ptr);
		if (rstatus) {
			fprintf(stderr, "Command Line Personality Function returns %d ... aborting\n",
				rstatus);
			return 1;
		}
	}
	// Perform parsing of the command line arguments
	// This will obtain the values for -m, -p, -b options
	// (-h, -m, -p and -b options)
	if (parse_clargs(argc, argv)) {
		fprintf(stderr, "Error parsing options ... aborting!\n");
		cmdarg_show_help(NULL);
		return 1;
	}

	if (NULL != ssrvrp->ipf) {
		rstatus = (*ssrvrp->ipf)(data_ptr);
		if (rstatus) {
			fprintf(stderr, "Initialization Personality Function returns %d ... aborting\n",
				rstatus);
			return 1;
		}
	}
	
	// Now set up the listen socket
	listenfd = setup_listen_socket();
	
	// Set child signal handler
	if (set_signal(SIGCHLD, sig_child_handler) == SIG_ERR) {
		fprintf(stderr, "Error registering SIGCHLD handler ... Aborting\n");
		return 1;
	}
	
	// Set termination signal handler
	if (set_signal(SIGTERM, sig_term_handler) == SIG_ERR) {
		fprintf(stderr, "Error register SIGTERM handler ... Aborting \n");
		return 1;
	}
	
	// Accept connections
	
	for (loop_abort = 0; !loop_abort; ) {
		socklen_t clasize;
		
		clasize = (socklen_t)sizeof(clientaddr);
		if (!listenfd_forced_closed) {
			connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clasize);
		} else {
			break;		// break out of the loop
		}
		if (connfd < 0) {
			if (EINTR == errno) {
				continue;
			} else if (!listenfd_forced_closed)  {
				fprintf(stderr, "accept error! errno = %d | %s\n", errno, strerror(errno));
				retstat = errno;
				break;
			}
		}
		
		if (listenfd_forced_closed) {
			break;			// break out of the for loop
		}
		
		if (open_conns >= max_open_conns) {
			// "Throttle" by closing the socket connection.
			// We'll hang another accept ... but no connection will
			// be processed until a child process terminates.
			close(connfd);
			continue;
		}
		open_conns++;		// increment counter of open connections
		
		if ((childpid = fork()) == 0) {
			int childstatus;
			// This is the child
			close(listenfd);
			
			// Invoke the main personality function if it is defined
			if (NULL != ssrvrp->mpf) {
				childstatus = (*ssrvrp->mpf)(connfd, data_ptr);
				
			} else {
				childstatus = 0;
			}
			// Invoke the child termination personality function
			if (NULL != ssrvrp->ctpf) {
				(*ssrvrp->ctpf)(connfd, data_ptr);
			}
			exit(childstatus);
		} else {
			// This is the parent. Close connfd to decrement reference count
			close(connfd);
		}
	}
	if (!listenfd_forced_closed) {
		close(listenfd);
	}
	if (NULL != ssrvrp->ptpf) {
		retstat =(*ssrvrp->ptpf)(data_ptr);
	}
	return retstat;	 
	
}
