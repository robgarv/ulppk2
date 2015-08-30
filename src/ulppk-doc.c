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
 * @file ulppk-doc.c
 *
 * @brief Simple utility that prints out library version information.
 */

/**
 * @mainpage
 *
 * @author Rob Garvey
 *
 * <h1>Introduction</h1>
 * The Unix/Linux Process Programming Kit (ULPPK --- hey I had to call it something)
 * is implemented as a shared library of C functions. These functions provide in
 * memory and memory mapped (persistent) data structure support, logging and
 * diagnostics, process control, socket communications, and other tools. These tools
 * provide a lightweight framework that supports rapid assembly of systems of
 * communicating processes. 
 *
 * The package ulppk-demo provides a simple demonstration system consisting of 
 * a command line client, a socket server, and a state machine process. The client sends
 * events to the socket server which dispatches them to the state machine process
 * via memory mapped message queue. 
 *
 * <h1>Background</h1>
 * ulppk is a packaging of a collection of re-used functions developed by the author (moi)
 * beginning in 1989. These functions were written to solve programming problems which
 * were recurring in systems development. Many of these functions were originally developed
 * for use in real time embedded applications and later ported to *nix systems.
 *
 * Over the years, a rather large but fairly disorganized "bag of tricks" had been
 * developed and carried from job to job. Extending and polishing the collection became
 * something of a hobby.
 *
 * ulppk packages the best and most frequently used items in this bag of tricks in
 * what is hopefully a coherent form. There are some rough edges ... particularly
 * with logging and certain aspects of ini file (program settings file) handling.
 * ulppk_log.c is useful but applications might well prefer to use other logging
 * applications which are more robust. Unfortunately, in its current state the library
 * itself has to use ulppk_log.c for logging. (I'm working on a "log provider" suite
 * to insulate the library code from the details of logging mechanisms but that
 * is still in the early stages of design.)
 *
 * Probably the most useful components of the library are the memory mapped I/O
 * modules and the state machine module. The state machine module offers a very
 * light weight facility for defining event driven processes modeled with Ward/Mellor
 * state transition diagrams. For many applications, this is rather much like
 * swatting a fly with a howitzer ... but if your analysis winds up defining more than
 * a dozen states and multiple transitions between states, this offers an implementation
 * that is far more manageable than the typical nested switch statement approach.
 *
 * Anyone who has read "Advanced Programming in the UNIX Environment" will recognize
 * the influence of the late W. Richard Stevens on this code. But then, almost everyone who
 * has programmed in the Unix/Linux environment remains indebted to Stevens. He
 * influenced us all.
 *
 * ulppk provides functions that support the following broad feature categories:
 *
 * <ul>
 * <li>In memory data structure support</li>
 * 	<ul>
 * 		<li>linked lists @see llacc.c</li>
 * 		<li>Double ended queues (stack/fifo) @see dqacc.c</li>
 * 		<li>Simple binary tree support @see btacc.c</li>
 * 	</ul>
 * <li>Application support</li>
 * 	<ul>
 * 		<li>Command line argument handling @see cmdargs.c</li>
 * 		<li>Debug output and diagnostics support @see diagnostics.c</li>
 * 		<li>Environment varible handling support @see appenv.c</li>
 * 		<li>Program settings (ini) file support @see ifile.c</li>
 * 		<li>Logging support @see ulppk_log</li>
 * 		<li>System configuration handling support @see sysconfig.c</li>
 * 		<li>File and path parsing/handling support @see pathinfo.c</li>
 * 		<li>I/O Utilities @see ioutils.c</li>
 * 	</ul>
 * <li>Unix/Linux Memory Mapped Datastructuring</li>
 * 	<ul>
 * 		<li>Memory Mapped File support @see mmatom.c</li>
 * 		<li>Memory Mapped File of Records support @see mmfor.c</li>
 * 		<li>Memory Mapped Linear Lists @see linearlist.c</li>
 * 		<li>Memory Mapped Double Ended Queue Support @see mmdeque.c</li>
 * 		<li>Memory Mapped Buffer Pool Support @see </li>
 * 	</ul>
 * <li>Process Management and Communications Support</li>
 * 	<ul>
 * 		<li>Socket support @see socketio.c</li>
 * 		<li>Socket Server Skeleton @see socketserver.c</li>
 * 		<li>Signal Handling Support @see signalkit.c</li>
 * 		<li>16 bit CRC support @see crc16ccitt.c </li>
 * 		<li>Process forking with throttle support @see process_control.c</li>
 * 		<li>State machine for event driven code @see statemachine.c</li>
 * 		<li>Process synchronization support @see msgcell.c</li>
 * 		<li>Process-to-process queing support @see msgdeque.c</li>
 * 	</ul>
 * </ul>
 *
 */
#include <config.h>
#include <stdio.h>

#include <ulppk.h>
#include <ulppk-properties.h>

int main(int argc, char* argv[]) {
	char* vlabel;

	printf("Package Name: \n\t%s\n", pkgname_string);
	printf("\tVersion: %s\n", PACKAGE_VERSION);
	printf("\tPackage Bug Report Email: %s\n", PACKAGE_BUGREPORT);
	printf("\tPackage Home Page: %s\n", PACKAGE_URL);
	vlabel = ulppk_version();
	printf("\tPackage Label: %s\n", vlabel);

	return 0;
}

