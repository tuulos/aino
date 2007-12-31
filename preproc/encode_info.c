/*
 *   preproc/build_info.c
 *   
 *   Copyright (C) 2008 Ville H. Tuulos
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

#include <dub.h>
#include <mmisc.h>

#include <serializer.h>
#include <dexvm.h>

#include "iblock.h"

static inline void encode_info(u32 did, doc_e *nfo)
{
        static toc_e toc;
        if (nfo){
                toc.offs = ftello64(data_f);
                fwrite(nfo, sizeof(doc_e), 1, data_f);
        }
        toc.val = did;
        fwrite(&toc, sizeof(toc_e), 1, toc_f);
}
        
int main(int argc, char **argv)
{
        const glist *seg = NULL;
        u32         key  = 0;
        u32         did  = 0;
        u32         sid  = 0;

        dub_init();
        init_iblock("info");
        open_serializer();
        
        while ((seg = pull_head(&key))){

                ENSURE_IBLOCK_PRE(did = sid = 0;)
                ENSURE_IBLOCK_POST()

                doc_e nfo = {
                        .key = key,
                        .first_sid = sid,
                        .prior = 1.0};

                encode_info(did, &nfo);
                push_head(key, seg);
                ++sid;
                
                while ((seg = pull_segment())){
                        encode_info(did, NULL);
                        push_segment(seg);
                        ++sid;
                }
                push_segment(NULL);

                ++did;
        }

        close_iblock();

        return 0;
}
