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

#ifndef SOCKETSERVER_H_
#define SOCKETSERVER_H_

/**
 * @file socketserver.h
 *
 * @brief A skeletal socket server framework.
 *
 * <h2>Overview</h2>
 * A generalized socket server. This model provides a skeletal framework
 * for the classic concurrent socket server. In that model, a client connects
 * to the listening server, which accepts the connection request and forks
 * a child to service the request.
 *
 * Applications code must provide the following "personality functions".
 * <ol>
 *	<li>a command line processing personality function (CLPF) to parse and handle application
 *		specific command line arguments.</li>
 *	<li>an initialization personality function (IPF) to handle initialization requirements
 *	<li>a main personality function (MPF) . This is the function that provides the
 *		applications logic performed by the child</li>
 *	<li>a child termination personality function (CTPF)</li>
 *	<li>a parent termination personality function</li>
 * </ol>

 * Any of these can be "stubbed out" by providing a null function pointer.
 *
 * <h2>COMMAND LINE ARGUMENTS</h2>
 *
 *	Any executable program that implements a socket server is presumed to
 *	require the following command line arguments:
 *
 *		-p [listen port number] -b [listen queue backlog]
 *
 *	The -b option is optional ... if not provided a backlog of 5 is presumed.
 *
 *	Other command line arguments may be optionally provided. Parsing of those
 * 	arguments is the function of the CLPF. The skeleton will handle parsing of
 * 	the required arguments.
 *
 * <h2>Using the skeletal socket server</h2>
 *
 * First, allocate a new socket server handle (structure) using ssrvr_new. This returns
 * returns a SSRVR_HANDLE which must be retained. Then, write and register the personality
 * functions using the registration functions provided. When this is complete,
 * call the function ssrvr_start. ssrvr_start will call the CLPF, the IPF, and then
 * set up the listen port. It will then block an accept call. When a connection is made,
 * the parent forks a child and blocks on another accept. The child executes the MPF.
 * When the (child) MPF completes, the skeleton calls the CTPF (Child termination
 * function). The parent termiantion personality function is called when the parent
 * terminates its main loop priot to exit.
 *
 * The socket server must accept the following
 * command line arguments (and these are handled by the skeleton):
 *
 * <ul>
 * <li>h -- command line help switch</li>
 * <li>b (--backlog) -- accept queue backlog limit (default is 5)</li>
 * <li>m (--maxconns) -- max open connections (default is 64)</li>
 * <li>p (--port) -- listen port (default is 49152)</li>
 * </ul>
 *
 * The calling program should register it's application
 * specific command line arguments by implementing a
 * Command Line Personality Function (clpf).
 *
 * ssrvr_start will call the clpf in the appropriate
 * sequence. The calling app must not call cmdarg_init
 * or cmdarg_parse. Again, this will be managed by
 * ssrvr_start.
 *
 */
#include "signalkit.h"

/**
 *Init personality function (IPF)
 */
typedef int  SSRVR_IPF(void* pData);

/**
 * Command Line Processing Personality function (CLPF).
 */
typedef int SSRVR_CLPF(int argc, char* argv[], void* pData);

/**
 * Main Personality Function (MPF)
 */
typedef int SSRVR_MPF(int connfd, void* pData);

/**
 *  Child termination personality function
 */
typedef int SSRVR_CTPF(int connfd, void* pData);

/**
 *  Parent termination personality function
 */
typedef int SSRVR_PTPF(void* pData);

/**
 *  Socket server handle structure.
 */
typedef struct {
	SSRVR_IPF	*ipf;		///< Function to IPF function
	SSRVR_CLPF	*clpf;		///< Function Pointer to CLPF
	SSRVR_MPF	*mpf;		///< Function pointer to MPF
	SSRVR_CTPF	*ctpf;		///< Function pointer to CTPF
	SSRVR_PTPF	*ptpf;		///< Function pointer to PTPF
} SSRVR_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif


// Functions to set up and start a socket server.

// This function constructs a new socket server handle structure.
// Personality functions are initialize to stubs.
SSRVR_HANDLE* ssrvr_new();

// Once a handle is created, personality functions may be registered
void ssrvr_register_ipf(SSRVR_HANDLE* pHandle, SSRVR_IPF *ipf);
void ssrvr_register_clpf(SSRVR_HANDLE* pHandle, SSRVR_CLPF *clpf);
void ssrvr_register_mpf(SSRVR_HANDLE* pHandle, SSRVR_MPF *mpf);
void ssrvr_register_ctpf(SSRVR_HANDLE* pHandle, SSRVR_CTPF *ctpf);
void ssrvr_register_ptpf(SSRVR_HANDLE* pHandle, SSRVR_PTPF *ptpf);

// Once the required personality functions have been registered, the
// socket server can be started.
// datap is arbitrary data provided by the application. It will be passed 
// to the personality functions on invocation (except for the child termination
// signal handler.)

int ssrvr_start(SSRVR_HANDLE* pHandle, void* datap, int argc, char* argv[]);

#ifdef __cplusplus
}
#endif

#endif /*SOCKETSERVER_H_*/
