/*
 *****************************************************************

<GPL>

Copyright: © 2001-2012 Robert C Garvey

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

#include <diagnostics.h>

int main(int argc, char* argv[]) {

	char * msgp;

	printf("test-diagnostics\n");

	msgp = ERR_MSG(NULL, "This is a message [%d] [%s] [%x]", 15, " is in hex ", 15);
	printf(msgp);

	printf("Intentionally aborting the application. Will cause a test failure to be posted.\n");
	printf("Success is really based on the error message that is displayed below:\n\n\n");
	printf("=======================================\n\n");
	APP_ERR(stdout, "Here is another error [%s] ErrorNum = [%d] FOOBAR", "Error text --- Error Num should be 77", 77);
	printf("\n=======================================\n");
	return 0;
}

