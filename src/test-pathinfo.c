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

/*
 * test-pathinfo.c
 *
 *  Created on: Dec 15, 2012
 *      Author: robgarv
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pathinfo.h>

char* append2path(char* path1, char* path2) {
	size_t bufflen;
	char* buff;

	bufflen = strlen(path1) + strlen(path2) + 2;
	buff = (char*)calloc(bufflen, sizeof(char));
	strcpy(buff, path1);
	strcat(buff, "/");
	strcat(buff, path2);
	return buff;
}

void getpathinfo(char* pathname) {
	PATHINFO_STRUCT* ps1p;
	char* pinfostring;

	ps1p = pathinfo_parse_filepath(pathname);
	pinfostring = pathinfo_tostring(ps1p);
	printf("%s\n", pinfostring);
	free(pinfostring);
	pathinfo_release(ps1p);

}

int main(int argc, char* argv[]) {
	char* workdir;
	char* ddir;
	char* testpath;

	workdir = get_current_dir_name();
	ddir = append2path(workdir, "../testdata");
	printf("Working directory is: %s\n", workdir);
	printf("Test data dir is %s\n", ddir);

	testpath = append2path(ddir,"test1.txt");
	getpathinfo(testpath);
	free(testpath);

	testpath = append2path(ddir, "test1.txt.ext");
	getpathinfo(testpath);
	free(testpath);

	testpath = append2path(ddir, ".hiddentest.txt");
	getpathinfo(testpath);
	free(testpath);

	testpath = append2path(ddir, "test1.dir");
	if (pathinfo_is_dir(testpath)) {
		getpathinfo(testpath);
		free(testpath);
	} else {
		printf("The path %s is not a directory. Expected a directory\n", testpath);
		free(testpath);
		return 1;
	}

	testpath = append2path(ddir, "test2.dir/test2.txt");
	getpathinfo(testpath);
	free(testpath);

	testpath = append2path(ddir, "afile");
	getpathinfo(testpath);
	free(testpath);

	testpath = append2path(ddir, ".bfile");
	getpathinfo(testpath);
	free(testpath);


	free(workdir);
	free(ddir);
	return 0;
}
