/*
 * efone - Distributed internet phone system.
 *
 * (c) 1999,2000 Krzysztof Dabrowski
 * (c) 1999,2000 ElysiuM deeZine
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

/* based on implementation by Finn Yannick Jacobs. */

#ifndef CRC32_H_
#define CRC32_H_

void chksum_crc32gentab (void);
u_int32_t chksum_crc32 (unsigned char *block, unsigned int length);
extern u_int32_t crc_tab[256];

#endif /* CRC32_H_ */
