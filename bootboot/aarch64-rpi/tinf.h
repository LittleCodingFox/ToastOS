/*
 * uzlib  -  tiny deflate/inflate library (deflate, gzip, zlib)
 *
 * Copyright (c) 2003 by Joergen Ibsen / Jibz
 * All Rights Reserved
 * http://www.ibsensoftware.com/
 *
 * Copyright (c) 2014-2016 by Paul Sokolovsky
 */

#ifndef TINF_H_INCLUDED
#define TINF_H_INCLUDED

/* we don't have stdint.h */
typedef signed char         int8_t;
typedef short int           int16_t;
typedef int                 int32_t;
typedef long int            int64_t;
typedef unsigned char       uint8_t;
typedef unsigned short int  uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long int   uint64_t;

/* calling convention */
#ifndef TINFCC
 #ifdef __WATCOMC__
  #define TINFCC __cdecl
 #else
  #define TINFCC
 #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ok status, more data produced */
#define TINF_OK             0
/* end of compressed stream reached */
#define TINF_DONE           1
#define TINF_DATA_ERROR    (-3)
#define TINF_CHKSUM_ERROR  (-4)
#define TINF_DICT_ERROR    (-5)

/* checksum types */
#define TINF_CHKSUM_NONE  0
#define TINF_CHKSUM_ADLER 1
#define TINF_CHKSUM_CRC   2

/* data structures */

typedef struct {
   unsigned int table[16];  /* table of code length counts */
   unsigned int trans[288]; /* code -> symbol translation table */
} TINF_TREE;

struct TINF_DATA;
typedef struct TINF_DATA {
   TINF_TREE ltree; /* dynamic length/symbol tree */
   TINF_TREE dtree; /* dynamic distance tree */
   const unsigned char *source;
   /* If source above is NULL, this function will be used to read
      next byte from source stream */
   unsigned char (*readSource)(volatile struct TINF_DATA *data);

   unsigned int tag;
   unsigned int bitcount;

    /* Buffer start */
    unsigned char *destStart;
    /* Buffer total size */
    unsigned int destSize;
    /* Current pointer in buffer */
    unsigned char *dest;
    /* Remaining bytes in buffer */
    unsigned int destRemaining;

    /* Accumulating checksum */
    unsigned int checksum;
    unsigned int checksum_type;

    int btype;
    int bfinal;
    unsigned int curlen;
    int lzOff;
} TINF_DATA;

#define TINF_PUT(d, c) \
    { \
        *d->dest++ = c; \
    }

unsigned char TINFCC uzlib_get_byte(volatile TINF_DATA *d);

/* Decompression API */

void TINFCC uzlib_init(void);
void TINFCC uzlib_uncompress_init(TINF_DATA *d, void *dict, unsigned int dictLen);
int  TINFCC uzlib_uncompress(volatile TINF_DATA *d);
int  TINFCC uzlib_uncompress_chksum(TINF_DATA *d);

int TINFCC uzlib_zlib_parse_header(TINF_DATA *d);
int TINFCC uzlib_gzip_parse_header(TINF_DATA *d);

/* Compression API */

void TINFCC uzlib_compress(void *data, const uint8_t *src, unsigned slen);

/* Checksum API */

/* prev_sum is previous value for incremental computation, 1 initially */
uint32_t TINFCC uzlib_adler32(const void *data, unsigned int length, uint32_t prev_sum);
/* crc is previous value for incremental computation, 0xffffffff initially */
uint32_t TINFCC uzlib_crc32(const void *data, unsigned int length, uint32_t crc);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TINF_H_INCLUDED */
