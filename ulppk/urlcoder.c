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
 * urlcoder.c
 *
 * This code is adapted from the code written by Fred Bullback, who
 * graciously "waived all copyright and related or
 * neighboring rights to URL Encoding/Decoding in C." The original
 * code can be found at:
 *
 * http://www.geekhideout.com/urlcode.shtml
 *
 * Fred's code is employed directly almost directly in the functons
 * 	o url_to_hex
 * 	o url_from_hex
 * 	o url_encode
 * 	o url_decode
 *
 *  Created on: Nov 11, 2012
 *      Author: robgarv
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>

#include <urlcoder.h>

/* Converts a hex character to its integer value */
char url_from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char url_to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) {
  char *pstr = str, *buf = malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
      *pbuf++ = *pstr;
    else if (*pstr == ' ')
      *pbuf++ = '+';
    else
      *pbuf++ = '%', *pbuf++ = url_to_hex(*pstr >> 4), *pbuf++ = url_to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(char *str) {
  char *pstr = str, *buf = malloc(strlen(str) + 1), *pbuf = buf;
  while (*pstr) {
    if (*pstr == '%') {
      if (pstr[1] && pstr[2]) {
        *pbuf++ = url_from_hex(pstr[1]) << 4 | url_from_hex(pstr[2]);
        pstr += 2;
      }
    } else if (*pstr == '+') {
      *pbuf++ = ' ';
    } else {
      *pbuf++ = *pstr;
    }
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}


// Take input str, write a URL encoded representation to buff.
// Return NULL on error. Otherwise, pointer to buff.
char* url_encode2buffer(char* buff, size_t buffsize, char* str) {
	char* encodestr;
	if (buffsize < (strlen(str) * 3 + 1)) {
		return NULL;
	}
	encodestr = url_encode(str);
	strcpy(buff, encodestr);
	free(encodestr);
	return buff;
}

// Take input str, write a URL decoded representation to buff.
// Return NULL on error. Otherwise, pointer to buff.
char* url_decode2buffer(char* buff, size_t buffsize, char* str) {
	char* decodestr;
	if (buffsize < (strlen(str) + 1)) {
		return NULL;
	}
	decodestr = url_decode(str);
	strcpy(buff, decodestr);
	return buff;
}

static void add_name_value(PLL_HEAD listp, char* name, char* value) {
	int bufflen;
	char* buffp;
	char* cp;
	char* decoded_value;
	PLL_NODE nodep;

	// Decode the URL encoded value
	decoded_value = url_decode(value);

	// Format the user data. Two strings, name then decoded value
	bufflen = strlen(name) + strlen(decoded_value) + 2;
	buffp = (char*)calloc(bufflen, sizeof(char));
	cp = buffp;
	strcpy(buffp, name);
	cp += strlen(name) + 1;
	strcpy(cp, decoded_value);

	// free the decoded value buffer
	free(decoded_value);

	// Allocate a linked list node
	nodep = ll_new_node(buffp, bufflen);
	ll_addback(listp, nodep);

	// Free the memory we used to build the buffer
	free(buffp);
}

/*
 * ll_search callback function.
 * Match on argument name. Argument name is the first string
 * found in the data buffer of the LL_NODE
 */
unsigned char match_arg_name(void* pdata, PLL_NODE currnodep) {
	char* databuffp;
	char* namep;
	unsigned char matchflag = 0;

	namep = (char*)pdata;
	databuffp = (char*)currnodep->data;

	// "name" is the first null terminated string packed
	// into the buffer
	if (strcmp(namep, databuffp) == 0) {
		matchflag = 1;
	}
	return matchflag;
}
/*
 * Return a pointer to the named parameter, NULL if not found.
 * The pointer should be considered "read only memory". It should
 * not be written to, or disposed of by the calling problem.
 * (Deallocation of memory is accomplished by url_free_arguments(listp)
 *
 * PLL_HEAD listp --- linked list of name/value pairs constructed by
 * char* name -- string containing the argument name
 * returns NULL if named name/value pair not found, otherwise pointer to value string.
 */
char* url_get_arg_value(PLL_HEAD listp, char* name) {
	char* valp = NULL;
	char* databuffp;
	PLL_NODE nodep;

	nodep = ll_search(listp, name, match_arg_name);
	if (nodep != NULL) {
		// Value will be the string packed into the data buffer
		// immediately following termination of the name string.
		databuffp = (char*)nodep->data;
		valp = databuffp + strlen(name) + 1;
	}
	return valp;
}

/*
 * Take a URL encoded argument list and decode its parts. In the URL
 *
 * http://www.example.com?myparam1="value1"&myparam2="value2"
 *
 * we are referring to everything to the right of the '?'
 *
 * The input is a string.
 *
 * The output is a linked list of name/value pairs. The name
 * value pairs are accessed by the url_get_arg_value(listp, name) function.
 * url_free_arguments(listp) must be called when you are done with
 * the set of name/value pairs. Otherwise, memory will be lost.
 *
 * listp -- ptr to linked list header object.
 * urlargs --- the input URL encoded argument list.
 * returns -- count of name/value pairs
 */
int url_decode_arguments(PLL_HEAD listp, char* urlargs) {
	char* cp;		// ptr into encoded arguments string
	char* lap;		// look ahead ptr
	char* namebuff;
	char* valbuff;
	char* np;
	char* vp;
	int state = 0;
	int count = 0;

	ll_init(listp);
	namebuff = (char*)calloc(strlen(urlargs), sizeof(char));
	valbuff = (char*)calloc(strlen(urlargs), sizeof(char));
	np = namebuff;
	vp = valbuff;

	/*
	 * state = 0 => accumulating a name (start or '&' triggers
	 * state = 1 => accumulating a value ('=' triggers)
	 */
	cp = urlargs;
	while (*cp != '\0') {
		if (state == 0) {
			// Currently accumulating a name
			if (*cp == '=') {
				// Now accumulating a value
				state = 1;
				cp++;			// pt to 1st char of value
				*vp = *cp;
				vp++;
			} else {
				// Still accumulating a name
				*np = *cp;
				np++;
			}
			cp++;
		} else if (state == 1) {
			// Currently accumulating a value
			lap = cp + 1;
			if ((*cp == '&') || (*lap == '\0')) {
				if (*lap == '\0') {
					// Store the last character in the value buffer
					*vp = *cp;
					vp++;
				}
				// Done with this accumulating name/value
				// Set state to 0 and add to list
				state = 0;
				count++;
				add_name_value(listp, namebuff, valbuff);

				// clear our working buffers and reset pointers
				memset(namebuff, 0, strlen(urlargs));
				memset(valbuff, 0, strlen(urlargs));
				np = namebuff;
				vp = valbuff;
			} else {
				*vp = *cp;
				vp++;
			}
			cp++;
		}
	}
	return listp->ll_cnt;
}

/*
 * Constructs a URL encoded argument name=value list.
 *
 * The return value is a pointer to a malloc'ed buffer
 * containing a character string representing the current
 * state of the argument name value list.
 *
 * On the first call to this function, pass NULL for args.
 * On subsequence calls, to this function, pass the
 * pointer returned by the previous call. For example:
 *
 * char* args;
 *
 * args = url_encode_arguments(NULL, "name1", "value1");
 * args = url_encode_arguments(args, "name2", "value2");
 * args = url_encode_arguments(args, "name3", "value3");
 *
 * <args will be name1="value1"&name2="value2"&name
 * < do something with the arg list>
 *
 * free(args);
 *
 * Note that if you do this:
 * args1 = url_encode_arguments(NULL, "name1", "value1");
 * args2 = url_encode_arguments(args1, "name2", "value2");
 *
 * <args1 has been returned to the heap and is invalid. args2 is valid.>
 *
 */
char* url_encode_arguments(char* args, char* name, char* val) {
	char* encval;
	char* newargs;

	encval = url_encode(val);
	int datalen = 0;

	datalen = strlen(name) + strlen(encval) + 3;
	if (NULL == args) {
		newargs = (char*)calloc(datalen, sizeof(char));
		sprintf(newargs, "%s=%s", name, encval);
	} else {
		datalen +=  strlen(args) + 1;
		newargs = (char*)calloc(datalen, sizeof(char));
		sprintf(newargs,"%s&%s=%s", args, name, encval);
	}
	if (args != NULL) {
		free(args);
	}
	return newargs;
}
/*
 * releases memory tied up in a argument list
 */
void url_free_arguments(PLL_HEAD listp) {
	ll_drain(listp, NULL);
}
