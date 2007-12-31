/*
 *   preproc/encode_inva.c
 *   constructs the inverted index
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

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include <Judy.h>

#include <pparam.h>
#include <ttypes.h>
//#include <pcode.h>
#include <dub.h>
#include <finnuio.h>
#include <ixemes.h>
//#include <bloom.h>
#include <mmisc.h>
#include <rrice.h>

#include <dexvm.h>
#include "defaults.h"
//#include "recode_docs.h"

#define INVA_BLOCK 268435456  /* 256MB */
#define IO_BUF_SIZE 1048576 /* 1MB */ 

static FILE *data_f;
static FILE *toc_f;

struct invaentry{
        uint          ix_freq;   
        growing_glist *list;
};

static void encode_invalist(const glist *g)
{
        static char *buf;
        static uint buf_len;

        u32 offs = 0;

        uint need = BITS_TO_BYTES(
                rice_encode_nolength(NULL, &offs, g->lst, g->len));
        if (need > buf_len){
                if (buf)
                        free(buf);
                buf_len = need;
                buf = xmalloc(buf_len);
                memset(buf, 0, buf_len);
        }
        offs = 0;
        rice_encode_nolength(buf, &offs, g->lst, g->len);
        offs = BITS_TO_BYTES(offs);

        fwrite(buf, offs, 1, data_f);
        memset(buf, 0, offs);
}

static void write_invalist(u32 ixid, growing_glist *g)
{
        toc_e toc;

        /* write toc entry */
        toc.val  = ixid;
        toc.offs = ftello64(data_f);
        fwrite(&toc, sizeof(toc_e), 1, toc_f);

        /* sanity check: make sure that we got all the entries */
        if (g->lst.len != g->sze)
                dub_die("Items missing from inverted list (ixid: %u)", ixid);
        
        /* INVALIST ENTRY */
        
        /* Ixeme frequency */
        fwrite(&g->sze, 4, 1, data_f);

        /* Invalist of segments */
        encode_invalist(&g->lst);
}

static Pvoid_t collect_document_frequencies()
{
        Pvoid_t occ = NULL;
        uint    i;
        
        for (i = 0; i < dex_section[FW].nof_entries; i++){

                const void *d   = fetch_item(FW, i);
                u32        offs = 0;
                u32        xid;

                DEX_FOREACH_VAL(d, &offs, xid)

                        Word_t *ix = NULL;
                        JLI(ix, occ, xid);
                        ++*ix;
                        
                DEX_FOREACH_VAL_END
        }
                
        return occ;
}


static void fill_invalists(Pvoid_t ix_occ, u32 min_ixid, u32 max_ixid)
{
        uint i;

        for (i = 0; i < dex_section[FW].nof_entries; i++){

                const void *d   = fetch_item(FW, i);
                u32        offs = 0;
                u32        ixid;
                
                DEX_FOREACH_VAL(d, &offs, ixid)

                        if (ixid >= min_ixid && ixid <= max_ixid){

                                growing_glist *g   = NULL;
                                Word_t        *ptr = NULL;
                        
                                JLG(ptr, ix_occ, (Word_t)ixid);
                                if (!ptr)
                                        dub_die("Unexpexted ixid %u", ixid);
                
                                /* add document to the list */
                                g = (growing_glist*)*ptr;
                                GGLIST_APPEND(g, i);
                        }
                        
                DEX_FOREACH_VAL_END
        }
}

static void build_inva(Pvoid_t ix_occ)
{
        Word_t *val = NULL;
        Word_t ixid = 0;
                
        u32 max_ixid = 0;
        u32 min_ixid = 0;
                
        uint inva_block = 0;
        uint block_sze  = 0;

        dub_msg("Building inverted index");
        
        PPARM_INT(inva_block, INVA_BLOCK);
               
        JLF(val, ix_occ, ixid);
        min_ixid = max_ixid = ixid; 
        
        /* Inverted index is built block by block. A block consists of ixemes
         * so that their total number of occurrences doesn't exceed inva_block.
         * For each block the forward index is looped through and dids are
         * collected. After this the did lists are pcoded and written to disk. 
         */
        while (val != NULL){
            
                growing_glist *list = NULL;
                GGLIST_INIT(list, *val);
                        
                block_sze += (*val + 2) * 4;

                max_ixid = ixid;
                *val = (Word_t)list;

                JLN(val, ix_occ, ixid);
          
                /* block full or all ixemes processed? */
                if (block_sze > inva_block || val == NULL){
                
                        Word_t *pr = NULL;
                        Word_t idx = min_ixid;
                       
                        /* fill the inverted lists of this block by going 
                         * through the forward index */
                        dub_msg("Collecting documents for ixeme range %u-%u.", 
                                        min_ixid, max_ixid);
                        
                        fill_invalists(ix_occ, min_ixid, max_ixid);
                        
                        dub_msg("Encoding");
                        
                        /* encode and write the lists */
                        JLF(pr, ix_occ, idx);
                        while (pr != NULL && idx <= max_ixid){
                       
                                list = (growing_glist*)*pr;
                                write_invalist(idx, list);
                                free(list);
                                
                                JLN(pr, ix_occ, idx);
                        }
                        
                        min_ixid = max_ixid + 1;
                        block_sze = 0;
                }
        }
}

int main(int argc, char **argv)
{
        char    *basename = NULL;
        char    *tname    = NULL;
        char    *dname    = NULL;
        Pvoid_t occ       = NULL;
        uint    iblock    = 0;
       
        toc_e toc;
        
        dub_init();
        
        PPARM_INT_D(iblock, IBLOCK);

        basename = pparm_common_name("inva");
        asprintf(&dname, "%s.%u.sect", basename, iblock);
        asprintf(&tname, "%s.%u.toc.sect", basename, iblock);
        free(basename);
         
        if (!(data_f = fopen64(dname, "w+")))
                dub_sysdie("Couldn't open section %s", dname);
        if (!(toc_f = fopen64(tname, "w+")))
                dub_sysdie("Couldn't open section %s", tname);
        
        if (setvbuf(data_f, NULL, _IOFBF, IO_BUF_SIZE))
                dub_sysdie("Couldn't open input buffer");
        if (setvbuf(toc_f, NULL, _IOFBF, IO_BUF_SIZE))
                dub_sysdie("Couldn't open input buffer");
        
        open_section(FW, iblock);
        
        occ = collect_document_frequencies();
        build_inva(occ);
        
        toc.val  = 0;
        toc.offs = ftello64(data_f);
        fwrite(&toc, sizeof(toc_e), 1, toc_f);

        dub_msg("Done.");
        
        fclose(data_f);
        fclose(toc_f);
        
        return 0;
}
