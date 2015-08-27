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

#ifndef _TTYMODES_H
#define _TTYMODES_H


int tty_cbreak(int fd);      /* put terminal into a cbreak mode */
int tty_raw(int fd);         /* put terminal into a raw mode */
int tty_reset(int fd);               /* restore terminal's mode */
void tty_atexit(void);                /* can be set up by atexit(tty_atexit) */
struct termios * tty_termios(void);   /* let caller see original tty state */

#endif

