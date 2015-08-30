
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
 * @file mmrpt_deque.c
 *
 * @brief Helper functions for reporting/dumping memory mapped deques.
 */
#include <stdio.h>
#include <string.h>

#include "mmrpt_deque.h"
#include <dqacc.h>
#include <rpt_deque.h>

/**
 * @brief Contruct a deque status report string and write to a buffer.
 *
 * This function is slightly dangerous in that the caller must insure
 * a sufficiently sized buffer is provide 512 bytes will certainly
 * be enough.
 *
 * @param buff Pointer to a sufficiently sized buffer
 * @param mmahp Memory mapped atom handle of the memory mapped deque.
 * @return Pointer to buff.
 */
char* mmrpt_deque2str(char* buff, MMA_HANDLE* mmahp) {
	DQHEADER* dequep;
	char* buffp;
	
	buffp = buff;
	sprintf(buffp, "mmdeque Report: Deque Named: %s\n", mmahp->tag);
	buffp = buff + strlen(buff);
	sprintf(buffp, "mmdeque File: %s\n", mmahp->u.df_refp->str_pathname);
	buffp =  buff + strlen(buff);
	dequep = (DQHEADER*)mma_data_pointer(mmahp);
	sprintf(buffp, "mmdeque Buffer Index: %ld\n", dequep->dqbuffx);
	buffp = buff + strlen(buff);
	rpt_deque2str(buffp, dequep);
	return buff;
}

/**
 * @brief Contruct a deque status report string and write to a buffer.
 *
 * @param f FILE* of stream to which report is to be written
 * @param mmahp Memory mapped atom handle of the memory mapped deque.
 * @return Point to output FILE.
 */
FILE* mmrpt_deque2file(FILE* f, MMA_HANDLE* mmahp) {
	char buff[512];
	fprintf(f, "%s\n", mmrpt_deque2str(buff, mmahp));
	return f;
}
