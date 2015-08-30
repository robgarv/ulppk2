
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
#ifndef MMRPT_DEQUE_H_
#define MMRPT_DEQUE_H_

/**
 * @file mmrpt_deque.h
 *
 * @brief Declarations for memory mapped deque status reporting.
 * functions.
 */
#include <stdio.h>
#include <mmatom.h>

#ifdef __cplusplus
extern "C" {
#endif

char* mmrpt_deque2str(char* buff, MMA_HANDLE* mmahp);
FILE* mmrpt_deque2file(FILE* f, MMA_HANDLE* mmahp);

#ifdef __cplusplus
}
#endif

#endif /*MMRPT_DEQUE_H_*/
