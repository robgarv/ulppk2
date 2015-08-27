
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
#include <stdio.h>
#include "rpt_deque.h"

char* rpt_deque2str(char* buff, DQHEADER* dequep) {
	char* dequetype;
	if (dequep->memmapped) {
		dequetype = "MEMMAPPED";
	} else if (dequep->buffctrl) {
		dequetype = "EXTERNAL BUFFER";
	} else {
		dequetype = "INTERNAL BUFFER";
	}
	sprintf(buff, "dq_open: %c  dq_slots: %d  dq_use: %d pct_use: %g\n"
				  "dqitem_size: %d             %s\n",
			((dequep->dq_open) ? 'T' : 'F'), dequep->dqslots, dequep->dquse, 
			(100.0 * dequep->dquse ) / dequep->dqslots,
			dequep->dqitem_size, dequetype);
	return buff;
}

void rpt_deque2file(FILE* f, DQHEADER* dequep) {
	char buff[512];
	fprintf(f, "%s\n", rpt_deque2str(buff, dequep));
}
