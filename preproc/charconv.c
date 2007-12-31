/*
 *   preproc/charconv.c
 *   Converts anything to latin-1 (possibly lossily). 
 *   
 *   Copyright (C) 2005-2008 Ville H. Tuulos
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>
#include <stdlib.h>

#include <dub.h>
#include <pparam.h>

#include "rawstream.h"
#include "charconv_magic.h"

#define MAX_DOC_SIZE 5242880 /* 5Mb */
#define KEEP_ALL     0
#define ENABLE_UTF16 1

static const char *conv(iconv_t ico, const char *buf, size_t len)
{
        static char *outbuf;
        static uint max_len;
        
        size_t left = 0;
        char   *out = NULL;
        
        if (max_len < len){
                if (outbuf)
                        free(outbuf);
                outbuf  = xmalloc(len + 1);
                max_len = len;
        }
        
        left = max_len;
        out  = outbuf;
        
        while (len){
       
                // iconf won't modify buf, casting is safe (?)
                if (iconv(ico, (char**)&buf, &len, &out, &left) == -1){
                        dub_dbg("Iconv conversion error: %d", errno);
                        break;
                }
                
                if (!left)
                        dub_die("Buffer ran empty. Strange.");
        }

        *out = 0;
        return outbuf;
}

int main(int argc, char **argv)
{
        struct rawdoc *doc;
        
        iconv_t  utf8conv;
        uint     max_size; 
        uint     keep_all;
        
#if ENABLE_UTF16
        iconv_t  utf16conv_le;
        iconv_t  utf16conv_be;
        int      res;
#endif

        dub_init();

        PPARM_INT(max_size, MAX_DOC_SIZE);
        PPARM_INT(keep_all, KEEP_ALL);

        if ((utf8conv = iconv_open("LATIN1", "UTF8")) == (iconv_t)-1)
                dub_sysdie("Couldn't open iconv for utf-8.");
        
#if ENABLE_UTF16
        if ((utf16conv_le = iconv_open("LATIN1", "UTF16LE")) == (iconv_t)-1)
                dub_sysdie("Couldn't open iconv for utf-16 (little endian).");
        
        if ((utf16conv_be = iconv_open("LATIN1", "UTF16BE")) == (iconv_t)-1)
                dub_sysdie("Couldn't open iconv for utf-16 (big endian).");
#endif
        
        while ((doc = pull_document())){

                const char *out = NULL;
               
                if (!keep_all && doc->body_size > max_size){
                        dub_dbg("too big");
                        continue;
                }
                
                /* Order of the if clauses matters! 
                 * See the original ascmagic.c */
                if (looks_ascii(doc->body, doc->body_size)){
                        dub_dbg("ascii");
                        out = doc->body;
                        
                }else if (looks_utf8(doc->body, doc->body_size)){
                        dub_dbg("utf8");
                        out = conv(utf8conv, doc->body, doc->body_size);

#if ENABLE_UTF16
                }else if ((res = looks_unicode(doc->body, doc->body_size))){
                        if (res & LOOKS_BIG_ENDIAN){
                                dub_dbg("utf16be");
                                out = conv(utf16conv_be, doc->body, 
                                                doc->body_size);
                        }else{
                                dub_dbg("utf16le");
                                out = conv(utf16conv_le, doc->body, 
                                                doc->body_size);
                        }
#endif
                
                }else if (looks_latin1(doc->body, doc->body_size)){
                        dub_dbg("latin1");
                        out = doc->body;
                
                }else if (looks_extended(doc->body, doc->body_size)){
                        /* what should be done here? */
                        dub_dbg("ext");
                        out = doc->body;
                        
                }else{
                        dub_dbg("unknown");

                        if (keep_all){
                                doc->body = NULL;
                                doc->body_size = 0;
                                push_document(doc);
                        }
                        continue;
                }
                
                doc->body = out;
                doc->body_size = strlen(out);
                push_document(doc);
        }
    
        return 0;
}



