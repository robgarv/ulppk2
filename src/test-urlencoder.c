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

/*
 * test-urlencoder.c
 *
 *  Created on: Nov 11, 2012
 *      Author: robgarv
 */

#include <stdio.h>
#include <stdlib.h>
#include <urlcoder.h>

int test_get_arg_value(PLL_HEAD listp, char* name, char* expected_val) {
	char* valp;

	valp = url_get_arg_value(listp, name);
	if (valp != NULL) {
		if (strcmp(expected_val, valp) != 0) {
			printf("Argument: %s expected value [%s] received value [%s]\n", name, expected_val, valp);
			exit(1);
		}
	} else {
		if (expected_val == NULL) {
			printf("Argument: %s expected value was NULL received NULL\n", name);
		} else {
			printf("Argument: %s not found when expected!\n", name);
		}
	}
	printf("name: %s decoded value: [%s]\n", name, valp);
}

int main(int argc, char* argv[]) {
	char* name1 = "name1";
	char* name2 = "name2";
	char* name3 = "name3";
	char* val1 = "val1";
	char* val2 = "val2 is a value with spaces";
	char* val3 = "val3	has a tab";
	char* url_args;
	LL_HEAD arglist;
	PLL_NODE frontp;
	PLL_NODE backp;
	int argcount;
	int teststatus = 0;
	char* valp;

	// Encode the arguments
	url_args = url_encode_arguments(NULL, name1, val1);
	url_args = url_encode_arguments(url_args, name2, val2);
	url_args = url_encode_arguments(url_args, name3, val3);

	printf("URL Encoded string: %s\n", url_args);

	// Now decode the arguments

	argcount = url_decode_arguments(&arglist, url_args);

	if (argcount != 3) {
		teststatus = 1;
		printf("Incorrect argument count: %d", argcount);
		exit(1);
	}
	test_get_arg_value(&arglist, name1, val1);
	test_get_arg_value(&arglist, name2, val2);
	test_get_arg_value(&arglist, name3, val3);

	exit(0);
}

