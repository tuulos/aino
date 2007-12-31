/*
 *   preproc/encode_docs.c
 *   encodes forward index
 *   
 *   Copyright (C) 2004-2008 Ville H. Tuulos
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dub.h>
//#include <pcode.h>
#include <mmisc.h>
#include <pparam.h>
#include <qexpansion.h>
#include <rrice.h>

#include <serializer.h>
#include <dexvm.h>

#include "iblock.h"

#define QEXP 0

static void encode_segment(const glist *seg, u32 val)
{
        static char *buf;
        static uint buf_len;
        
        toc_e toc;
        u32 offs = 0;

        toc.offs = ftello64(data_f);
        toc.val  = val;
        fwrite(&toc, sizeof(toc_e), 1, toc_f);
       
        uint need = BITS_TO_BYTES(rice_encode(NULL, 
                &offs, seg->lst, seg->len));
        if (need > buf_len){
                if (buf)
                        free(buf);
                buf_len = need;
                buf = xmalloc(buf_len);
                memset(buf, 0, buf_len);
        }
        offs = 0;
        rice_encode(buf, &offs, seg->lst, seg->len);
        offs = BITS_TO_BYTES(offs);

        fwrite(buf, offs, 1, data_f);
        memset(buf, 0, offs);
}

int main(int argc, char **argv)
{
        const glist *seg = NULL;
        u32         key  = 0;
        u32         sid  = 0;

        uint do_qexp;
        
        dub_init();
        init_iblock("fw");
        open_serializer();
               
        PPARM_INT(do_qexp, QEXP);
        if (do_qexp){
                char *fname = pparm_common_name("qexp");
                load_qexp(fname, 0);
        }

        while ((seg = pull_head(&key))){
       
                glist *exp = NULL;
                
                ENSURE_IBLOCK_PRE(sid = 0;)
                ENSURE_IBLOCK_POST()

                if (qexp_loaded)
                        seg = exp = expand_all(seg);
                        
                encode_segment(seg, sid);
                push_head(key, seg);
                ++sid;

                if (exp)
                        free(exp);
                
                while ((seg = pull_segment())){
                        
                        if (qexp_loaded)
                                seg = exp = expand_all(seg);
                        
                        encode_segment(seg, sid); 
                        push_segment(seg);
                        ++sid;

                        if (exp)
                                free(exp);
                }
                push_segment(NULL);
        }

        close_iblock();

        return 0;
}
