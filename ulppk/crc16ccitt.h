#ifndef _CRC16CCITT_H
#define _CRC16CCITT_H

/**
 * @file crc16ccitt.h
 *
 * @brief Simple utility for calculating a 16 bit CRC
 */


#ifdef __cplusplus
extern "C" {
#endif


// Must be called before calculating CRC over a sequence of bytes
void init_crc_ccitt();

// Given a byte of data, update the running CRC calculation

unsigned short crc16ccitt(char data_byte);

#ifdef __cplusplus
}
#endif

#endif
