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

#include "crc16ccitt.h"

/**
 * @file crc16ccitt.c
 *
 * @brief Simple utility for calculating a 16 bit CRC
 */

static unsigned int crc;
static int crc_tabccitt_init = 0;
static unsigned short   crc_tabccitt[256];
#define P_CCITT     0x1021

/*
 * ******************************************************************
 *
 *   static void init_crcccitt_tab( void );
 *
 *   The function init_crcccitt_tab() is used to fill the  array
 *   for calculation of the CRC-CCITT with values. 
 *
 ******************************************************************
 */

static void init_crcccitt_tab( void ) {

    int i, j;
    unsigned short crc, c;

    for (i=0; i<256; i++) {

        crc = 0;
        c   = ((unsigned short) i) << 8;

        for (j=0; j<8; j++) {

            if ( (crc ^ c) & 0x8000 ) crc = ( crc << 1 ) ^ P_CCITT;
            else                      crc =   crc << 1;

            c = c << 1;
        }

        crc_tabccitt[i] = crc;
    }

    crc_tabccitt_init = 1;
}

static unsigned short update_crc_ccitt( unsigned short crc, char c ) {

    unsigned short tmp, short_c;

    short_c  = 0x00ff & (unsigned short) c;

    if ( ! crc_tabccitt_init ) init_crcccitt_tab();

    tmp = (crc >> 8) ^ short_c;
    crc = (crc << 8) ^ crc_tabccitt[tmp];

    return crc;

}  /* update_crc_ccitt */




/**
 *
 * @brief Given a byte of data, update the running CRC calculation.
 *   The function update_crc_ccitt calculates  a  new  CRC-CCITT
 *   value  based  on the previous value of the CRC and the next
 *   byte of the data to be checked.
 *
 *   @param data_byte Byte to add into the running CRC
 *   @return new 16 bit CRC value.
 *
*/
unsigned short crc16ccitt(char data_byte) {
	crc = update_crc_ccitt(crc, data_byte);
	return crc;
}

/**
 * Initializes the crc tables.
 */
void init_crc_ccitt() {
	crc = 0;
}

