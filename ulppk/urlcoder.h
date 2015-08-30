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
 * urlcoder.h
 *
 *  Created on: Nov 11, 2012
 *      Author: robgarv
 */

#ifndef URLCODER_H_
#define URLCODER_H_

#include <stddef.h>

#include <llacc.h>

/* Converts a hex character to its integer value */
char url_from_hex(char ch);
/* Converts an integer value to its hex character*/
char url_to_hex(char code);
/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str);
char *url_decode(char *str);

/*
 * Wrappers. These functions presume that buff is a character
 * array allocated on the stack or static or global memory. (I.e.
 * not obtained from the heap by malloc or other means.) These
 * forms are "safer" because you don't have to remember to
 * free memory returned by the functions.
 *
 */
// Take input str, write a URL encoded representation to buff.
// Return NULL on error. Otherwise, pointer to buff.
char* url_encode2buffer(char* buff, size_t buffsize, char* str);
// Take input str, write a URL decoded representation to buff.
// Return NULL on error. Otherwise, pointer to buff.
char* url_decode2buffer(char* buff, size_t buffsize, char* str);


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
 * value pairs are accessed by the url_get_arg_value(name) function.
 * url_free_arguments(listp) must be called when you are done with
 * the set of name/value pairs. Otherwise, memory will be lost.
 *
 * listp -- ptr to linked list header object.
 * urlargs --- the input URL encoded argument list.
 * returns -- count of name/value pairs
 */
int url_decode_arguments(PLL_HEAD listp, char* urlargs);

/*
 * Return a pointer to the named parameter, NULL if not found.
 * The pointer should be considered "read only memory". It should
 * not be written to, or disposed of by the calling problem.
 * (Deallocation of memory is accomplished by url_free_arguments(listp)
 *
 * PLL_HEAD listp --- linked list of name/value pairs constructed by
 * char* name -- string containing the argument name
 * returns NULL if error, otherwise pointer to value string.
 */
char* url_get_arg_value(PLL_HEAD listp, char* name);

/*
 * releases memory tied up in a argument list
 */
void url_free_arguments(PLL_HEAD listp);

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
char* url_encode_arguments(char* args, char* name, char* val);

#endif /* URLCODER_H_ */
