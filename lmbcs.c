#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <conio.h>
#include <dos.h>

#include "config.h"
#include "debug.h"


static const unsigned char lmbcs_group1_table[256] = {
    0xB1, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0xB1, 0x7E, 0xF8, 0x5E, 0x60, 0x27, 0x22, 0x27,
    0xB1, 0x2D, 0xC4, 0xB1, 0xB1, 0xB1, 0x3C, 0x3E,
    0xB1, 0x7E, 0xF8, 0x5E, 0x60, 0x27, 0xB1, 0x2C,
    0x22, 0xB1, 0xB1, 0xB1, 0xB1, 0xB1, 0xB1, 0xB1,
    0xB1, 0xB1, 0x98, 0xB1, 0xB1, 0xB1, 0xC6, 0xC7,
    0xDD, 0xDE, 0xB1, 0xB1, 0xB1, 0xB1, 0xB1, 0xB1,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
    0xD8, 0xB5, 0xB6, 0xB7, 0xB8, 0xBD, 0xBE, 0xCF,
    0xB1, 0xB1, 0xB1, 0xB1, 0xFC, 0x6C, 0x4C, 0xB1,
    0xB1, 0xB1, 0xB1, 0xB1, 0xB1, 0xB1, 0xB1, 0xB1,
    0xB1, 0xB1, 0xB1, 0xB1, 0xB1, 0xB1, 0xB1, 0xB1,
    0xB1, 0xB1, 0xB1, 0xB1, 0x4B, 0xA9, 0x9C, 0x9E,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9A, 0x6F, 0x9C, 0x4F, 0x78, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
    0xA8, 0x54, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0x41, 0x41, 0x41,
    0x63, 0xB9, 0xBA, 0xBB, 0xBC, 0x9B, 0x9D, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0x61, 0x41,
    0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xB1,
    0x64, 0x44, 0x45, 0x45, 0x45, 0x69, 0x49, 0x49,
    0x49, 0xD9, 0xDA, 0xDB, 0xDC, 0x7C, 0x49, 0xDF,
    0x4F, 0xE1, 0x4F, 0x4F, 0x6F, 0x4F, 0xE6, 0x70,
    0x50, 0x55, 0x55, 0x55, 0x79, 0x59, 0x5F, 0x27,
    0x2D, 0xF1, 0x3D, 0xB1, 0x14, 0x15, 0xF6, 0x2C,
    0xF8, 0x22, 0xF9, 0x31, 0x33, 0xFD, 0xFE, 0xFF
};

// Add more code pages here, as needed, these are these tables:
// https://en.wikipedia.org/wiki/Lmbcs#Group_1
static const unsigned char * lmbcs_tables[31] = {
    NULL,                   // 0
    lmbcs_group1_table,     // 1
};

// This is the able from https://en.wikipedia.org/wiki/Lmbcs#Encodings
static const int lmbcs_group_length[] = {
    1, // NUL
    2, // Code page 850 (DOS Latin-1)
    2, // Code page 851 (DOS Greek)
    2, // Code page 1255 (Windows Hebrew)
    2, // Code page 1256 (Windows Arabic)
    2, // Code page 1251 (Windows Cyrillic)
    2, // Code page 852 (DOS Latin-2)
    1, // BEL
    2, // Code page 1254 (Windows Turkish)
    1, // TAB
    1, // LF
    2, // Code page 874 (Thai)
    2, // Reserved
    1, // CR
    2, // Reserved
    2, // Remapped C0/C1 control codes
    3, // Code page 932/
    3, // Code page 949/
    3, // Code page 950
    3, // Code page 936/
    3, // UTF-16 (Unicode)
    3, // Reserved
    3, // Reserved
    3, // Reserved
    3, // Reserved
    1, // Lotus 1-2-3 system range
    3, // Reserved
    3, // Reserved
    3, // Reserved
    3, // Reserved
    3, // Reserved
    3, // Reserved
};

// maxlen == 0 means count chars, but dont write.
int translate_lmbcs(const void *src,
                    unsigned char *dst,
                    int maxdst,
                    int maxsrc,
                    int defgroup)
{
    const unsigned char *p; // current lmbcs byte position
    int count;              // how many characters weve translated

    p = src;

    trace("translate_lmbcs");

    traceint(maxdst);
    traceint(maxsrc);

    if (lmbcs_tables[defgroup] == NULL) {
        trace("no translation table for default group");
        __debugbreak();
    }

    if (maxsrc == -1) {
        maxsrc = strlen(src);

        traceint(maxsrc);
    }

    loghex(src, maxsrc);

    for (count = 0; p < (unsigned char *) src + maxsrc; count++) {

        // Literal character
        if (*p >= 0x20 && *p <= 0x7f) {
            if (count < maxdst) {
                *dst++ = *p;
            }
            p++; // 1byte
            continue;
        }

        // Nul terminator
        if (*p == 0x00) {
            if (count < maxdst) {
                *dst++ = *p;
            }
            break;
        }

        if (*p < 0x20 && *p != 0x00) {
            if (lmbcs_tables[*p] == NULL) {
                logmsg("lmbcs string uses group %hd, no table available", *p);
                if (count < maxdst) {
                    *dst++ = '?';
                }
            } else {
                // Lookup in specified table.
                if (count < maxdst) {
                    *dst++ = lmbcs_tables[p[0]][p[1]];
                }
            }

            // Lookup how long this groups characters are.
            p += lmbcs_group_length[*p];
            continue;
        }

        // Lookup in default table.
        if (count < maxdst) {
            *dst++ = lmbcs_tables[defgroup][*p];
        }

        p++; // code
    }

    logmsg("translated characters is %d", count);
    return count;
}

