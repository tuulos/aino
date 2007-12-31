
#ifndef __CHARCONV_MAGIC__
#define __CHARCONV_MAGIC__

#define LOOKS_BIG_ENDIAN 2

int looks_ascii (const unsigned char *, int);
int looks_utf8 (const unsigned char *, int);
int looks_unicode (const unsigned char *, int);
int looks_latin1 (const unsigned char *, int);
int looks_extended (const unsigned char *, int);

#endif /* __CHARCONV_MAGIC__ */
