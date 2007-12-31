/*
 *   preproc/encode_pos.c
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dub.h>
#include <gglist.h>
#include <pparam.h>
#include <dexvm.h>
//#include <pcode.h>
#include <rrice.h>

#include "encode_pos.h"
#include "iblock.h"
#include "tok.h"

static uint encode_tokens(const Pvoid_t tokens)
{
        static growing_glist *tok;
        static char *buf;
        static uint buf_len;
        uint i, max_pos = 0;

        if (!tok){
                GGLIST_INIT(tok, 1000);
        }
        tok->lst.len = 0;
        Word_t *ptr;
        Word_t xid = 0; 
        JLF(ptr, tokens, xid);
        while (ptr){
                u32 x = xid;
                GGLIST_APPEND(tok, x);
                
                const growing_glist *g = (const growing_glist*)*ptr;
                for (i = 0; i < g->lst.len; i++)
                        if (g->lst.lst[i] > max_pos)
                                max_pos = g->lst.lst[i];

                JLN(ptr, tokens, xid);
        }

        u32 offs = 0;
        uint need = rice_encode(NULL, 
                &offs, tok->lst.lst, tok->lst.len);
        uint need_bytes = BITS_TO_BYTES(need);
        
        if (need_bytes > buf_len){
                free(buf);
                buf_len = need_bytes;
                buf = xmalloc(buf_len + 12);
                memset(buf, 0, buf_len + 12);
        }
        
        offs = 0;
        elias_gamma_write(buf, &offs, need);
        rice_encode(buf, &offs, tok->lst.lst, tok->lst.len);
        
        offs = BITS_TO_BYTES(offs);
        fwrite(buf, offs, 1, data_f);
        memset(buf, 0, offs);

        return max_pos;
}

void open_possect()
{        
        init_iblock("pos");
}

void close_possect()
{
        close_iblock();
}

/*
void write_pos(char *dest, u32 *offs, const growing_glist *g)
{
        int i;
        u32 prev_pos = 0;
        u32 max = 500;
        u32 f = estimate_rice_f(&max, 1);
        for (i = 0; i < g->lst.len; i++){
                rice_write(dest, offs, g->lst.lst[i] - prev_pos, f);
                dub_msg("VAL %u D %u", g->lst.lst[i], g->lst.lst[i] - prev_pos);
                prev_pos = g->lst.lst[i];
        }
        rice_write(dest, offs, 0, f);
}
*/

//void possect_add_old(u32 key, const Pvoid_t tokens);
void possect_add(const Pvoid_t tokens)
{
        static char *posbuf;
        static char *xidbuf;
                
        if (!posbuf){
                posbuf = xmalloc(iblock_size * 8 + 8);
                memset(posbuf, 0, iblock_size * 8 + 8);
                xidbuf = xmalloc((iblock_size * 2 + 3) * 8);
                memset(xidbuf, 0, (iblock_size * 2 + 3) * 8);
        }

        Word_t *ptr;
        u32 len, xoffs, poffs, prev_offs, prev_xid, 
                xsum, xmax, osum, omax;
        Word_t xid = poffs = prev_offs = prev_xid =\
                xsum = osum = osum = omax = xmax = len = 0;
        
        JLF(ptr, tokens, xid);
        while (ptr){
                growing_glist *g = (growing_glist*)*ptr;
                *ptr = poffs;
                rice_encode(posbuf, &poffs, g->lst.lst, g->lst.len);
                free(g);

                xsum += xid - prev_xid;
                osum += poffs - prev_offs;
                
                if (xid - prev_xid > xmax)
                        xmax = xid - prev_xid;
                if (poffs - prev_offs > omax)
                        omax = poffs - prev_offs;

                prev_xid = xid;
                prev_offs = poffs;

                ++len;
                JLN(ptr, tokens, xid);
        }
        poffs = BITS_TO_BYTES(poffs);
        
        xid = prev_xid = prev_offs = xoffs = 0;

        elias_gamma_write(xidbuf, &xoffs, len + 1);

        if (!len)
                goto write_buffers;

        u32 rice_f1 = estimate_rice_f_param(xsum, xmax, len);
        u32 rice_f2 = estimate_rice_f_param(osum, omax, len);
       
        rice_write(xidbuf, &xoffs, rice_f1, 2);
        rice_write(xidbuf, &xoffs, rice_f2, 2);

        JLF(ptr, tokens, xid);
        while (ptr){
                rice_write(xidbuf, &xoffs, xid - prev_xid, rice_f1);
                prev_xid = xid;
                rice_write(xidbuf, &xoffs, *ptr - prev_offs, rice_f2);
                prev_offs = *ptr;
                JLN(ptr, tokens, xid);
        }

write_buffers:
        xoffs = BITS_TO_BYTES(xoffs);
        
        toc_e toc;
        toc.offs = ftello64(data_f);
        toc.val = xoffs;
        fwrite(&toc, sizeof(toc_e), 1, toc_f);
        
        fwrite(xidbuf, xoffs, 1, data_f);
        fwrite(posbuf, poffs, 1, data_f);
        memset(xidbuf, 0, xoffs);
        memset(posbuf, 0, poffs);
}

#if 0
/* This functions codes all ixeme occurrences in a segment in the correct 
 * order. The catch is that instead of using global ixeme IDs, we use a
 * minimal local set of IDs, namely integers [1..|S|] for |S| individual
 * ixemes. Mapping between the IDs is implictely saved in the ascending 
 * order of ixeme IDs, obtainable from the forward index. */
void possect_add(const Pvoid_t tokens)
{
        static uint *offsets;
        //static uint offsets_len;
        static char *encode_buf;
        //static uint buf_len;
        
        if (!offsets){
                offsets = xmalloc(segment_size * sizeof(uint));
                encode_buf = xmalloc(segment_size * 8);
                memset(encode_buf, 0, segment_size * 8);
        }
        memset(offsets, 0, segment_size * sizeof(uint));

        //ENSURE_IBLOCK
        
        Word_t max = 0;
        JLC(max, tokens, 0, -1);

        /* write TOC entry */
        toc_e toc;
        toc.offs = ftello64(data_f);
        toc.val = max;
        fwrite(&toc, sizeof(toc_e), 1, toc_f);
        
        /* write token list */
        uint max_pos = encode_tokens(tokens);
        
        /*
        if (segment_size > offsets_len){
                free(offsets);
                offsets_len = segment_size;
                offsets = xmalloc(offsets_len * sizeof(uint));
        }
        memset(offsets, 0, offsets_len * sizeof(uint));
        */

        /* estimate rice_f */
        u32 rice_f, offs = 0;
        if (max)
                rice_f = estimate_rice_f((u32*)&max, 1);
        else
                rice_f = 2;

        /* allocate bits to positions */
        Word_t *ptr;
        //uint max_pos = 0;
        int i, j, c = 0;
        Word_t xid = 0; 
        JLF(ptr, tokens, xid);
        while (ptr){
                offs = 0;
                rice_write(NULL, &offs, ++c, rice_f);

                const growing_glist *g = (const growing_glist*)*ptr;
                for (i = 0; i < g->lst.len; i++){
                        offsets[g->lst.lst[i] - 1] = offs;
                        if (g->lst.lst[i] > max_pos)
                                max_pos = g->lst.lst[i];
                }

                JLN(ptr, tokens, xid);
        }
        
        /* compute bit offset for each position */
        offs = 0;
        elias_gamma_write(NULL, &offs, max_pos + 1);
        rice_write(NULL, &offs, rice_f, 2);
        for (i = 0; i < max_pos; i++){
                uint tmp = offsets[i];
                if (!tmp)
                        dub_die("A hole in poslist at %u / %u!", i, max_pos);
                offsets[i] = offs;
                offs += tmp;
        }
        u32 endpos = offs;
        
        //u32 total_bits = offs;
        //elias_gamma_write(NULL, &offs, total_bits);
        //rice_write(NULL, &offs, 0, rice_f);

        u32 total_bytes = BITS_TO_BYTES(offs);
        if (total_bytes > segment_size * 8)
                dub_die("Pos buffer too small! %u > %u.",
                        total_bytes, segment_size * 8);
        /* 
        if (total_bytes > buf_len){
                free(encode_buf);
                buf_len = total_bytes;
                encode_buf = xmalloc(buf_len + 4);
                memset(encode_buf, 0, buf_len + 4);
        }
        */

        /* write codes */

        offs  = 0;
        elias_gamma_write(encode_buf, &offs, max_pos + 1);
        rice_write(encode_buf, &offs, rice_f, 2);
        //elias_gamma_write(encode_buf, &offs, total_bits);

        xid = c = j = 0; 
        JLF(ptr, tokens, xid);
        while (ptr){
                ++c;
                growing_glist *g = (growing_glist*)*ptr;
                for (i = 0; i < g->lst.len; i++){
                        //u32 d = offsets[g->lst.lst[i] - 1];
                        //dub_msg("OFFS %u VAL %u", d, c);
                        rice_write(encode_buf, 
                                &offsets[g->lst.lst[i] - 1], c, rice_f);
                        //u32 dd = rice_read(encode_buf, &d, rice_f);
                        //if (c != dd) 
                        //        dub_die("POKS! %u, %u != %u", key, dd, c);
                        ++j;
                }
                free(g);
                JLN(ptr, tokens, xid);
        }
        
        //rice_write(encode_buf, &endpos, 0, rice_f);
        fwrite(encode_buf, total_bytes, 1, data_f);
        memset(encode_buf, 0, total_bytes);
}
#endif
#if 0
void possect_add(u32 id, Pvoid_t tokens)
{
        static growing_glist *ixlist;
        static glist         **lists;
        
        Word_t *ptr;
        Word_t idx     = 0;
        uint   ix_cnt  = 0;
        uint   i       = 0;

        toc_e toc;
      
        if (!ixlist){
                GGLIST_INIT(ixlist, 100);
        }
       
        
        /* write TOC entry */
        toc.offs = ftello64(data_f);
        toc.val  = id;
        fwrite(&toc, sizeof(toc_e), 1, toc_f);
        
        /* write ixeme list */
        ixlist->lst.len = 0;
        JLF(ptr, tokens, idx);
        while (ptr != NULL){
                ++ix_cnt;
                GGLIST_APPEND(ixlist, idx); 
                JLN(ptr, tokens, idx);
        }

        lists = xmalloc((ix_cnt + 1) * sizeof(glist*));
        lists[i++] = &ixlist->lst;
        
        /* write ixeme positions */
        idx = 0;
        JLF(ptr, tokens, idx);
        while (ptr != NULL){
                growing_glist *g = (growing_glist*)*ptr;
                lists[i++] = &g->lst;
                JLN(ptr, tokens, idx);
        }
                
        encode_poslists(lists, ix_cnt + 1);
     
        /* free stuff */
        idx = 0;
        JLF(ptr, tokens, idx);
        while (ptr != NULL){
                growing_glist *g = (growing_glist*)*ptr;
                free(g);
                JLN(ptr, tokens, idx);
        }
}
#endif
