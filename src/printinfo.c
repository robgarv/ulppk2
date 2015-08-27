
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#define PR_SYSCONF(f,n) pr_sysconf(f,#n, n);
 
static void pr_sysconf(FILE* f, char* mesg, int name);

int main(int argc, char* argv[]) {

	fprintf(stdout, "**** SYSTEM INFORMATION DUMP ****\n");
	PR_SYSCONF(stdout, _SC_ARG_MAX);
	PR_SYSCONF(stdout, _SC_CHILD_MAX);
	PR_SYSCONF(stdout, _SC_CLK_TCK);
	PR_SYSCONF(stdout, _SC_OPEN_MAX);
	PR_SYSCONF(stdout, _SC_PAGESIZE);
	
	{
		long val;
		
		val = pathconf("/", _PC_PATH_MAX);
		printf("MaxPath = %ld\n", val);
		printf("MAX_PATH = %d\n", PATH_MAX);
	}
	return 0;
}

static void pr_sysconf(FILE* f, char* mesg, int name) {

	long val;
	
	fputs(mesg, f);
	errno = 0;
	if ((val = sysconf(name)) < 0) {
		if (errno != 0) {
			fputs(strerror(errno), stderr);
			exit(1);
		}
		fputs(" not defined\n", stdout);
	} else {
		fprintf(f, " %ld\n", val);
	}
}
