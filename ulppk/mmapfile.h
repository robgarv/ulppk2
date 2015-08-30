
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
#ifndef MMAPFILE_H_
#define MMAPFILE_H_

#include <mmatom.h>

/**
 * @file mmapfile.h
 *
 * @brief Higher level functionality for creating, opening, closing,
 * and otherwise managing memory mapped atoms. Uses mmatom.c.
 *
 * First evolution of a memory mapped "atom" (mmatom). This specialization
 * represents a simple memory mapped file.
 */

#ifdef __cplusplus
extern "C" {
#endif
 
MMA_HANDLE* mmapfile_create(char* tag,
 	char* filepath, size_t len, MMA_ACCESS_MODES mode, 
 	MMA_MAP_FLAGS flags, int permissions);
MMA_HANDLE* mmapfile_open(char* tag, char* filepath, MMA_ACCESS_MODES mode,
 	MMA_MAP_FLAGS flags);
int mmapfile_close(MMA_HANDLE* mmahp);

char* mmapfile_file_path(MMA_HANDLE* mmahp);
size_t mmapfile_p2fileoffset(MMA_HANDLE* mmahp, void* p);

#ifdef __cplusplus
}
#endif
 
 
#endif /*MMAPFILE_H_*/
