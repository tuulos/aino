/*
 *   indexing/dexvm.c
 *   index mapping
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

#include <pparam.h>
#include <dub.h>
#include <finnuio.h>
#include <mmisc.h>
#include <time.h>
#include <rrice.h>

#include "dexvm.h"

#define VM_PAGE_SIZE 16777216 /* 16MB */

static uint page_size;

struct dex_section_s dex_section[DEX_NOF_SECT];
const struct header_s dex_header;
static void *dex_base_addr;
static uint dex_base_size;

static inline uint find_last(const struct dex_section_s *s, uint first);

void open_section(enum dex_sect_id sect, uint iblock_no)
{
        char *basename = pparm_common_name(pre_dex_names[sect]);
        open_section_i(basename, sect, iblock_no);
        free(basename);
}

void open_section_i(const char *basename, 
        enum dex_sect_id sect, uint iblock_no)
{
        int     toc_fd    = 0;
        int     data_fd   = 0;
        off64_t fpos      = 0;
        
        char  *dname    = NULL;
        char  *tname    = NULL;
        
        PPARM_INT(page_size, VM_PAGE_SIZE);

        asprintf(&dname, "%s.%u.sect", basename, iblock_no);
        asprintf(&tname, "%s.%u.toc.sect", basename, iblock_no);
        
        if ((data_fd = open(dname, O_RDONLY | O_LARGEFILE, 0)) == -1)
                dub_sysdie("Couldn't open section %s", dname);

        if ((toc_fd = open(tname, O_RDONLY)) == -1)
                dub_sysdie("Couldn't open toc %s", tname);
     
        memset(&dex_section[sect], 0, sizeof(struct dex_section_s));
        
        dex_section[sect].sect_fd   = data_fd;
        dex_section[sect].sect_offs = 0;
      
        if ((fpos = lseek64(toc_fd, 0, SEEK_END)) == (off64_t)-1)
                dub_sysdie("Couldn't seek toc %s", tname);
                
        dex_section[sect].nof_entries = fpos / sizeof(toc_e) - 1;

        if (!dex_section[sect].nof_entries)
                dub_die("TOC %s empty.", tname);
        
        dub_dbg("Mmapping %u bytes of toc.", fpos);
        
        dex_section[sect].toc = mmap64(0, fpos, PROT_READ, 
                                         MAP_SHARED, toc_fd, 0);
        
        if (dex_section[sect].toc == MAP_FAILED)
                dub_sysdie("Couldn't mmap toc %s", tname);

        free(dname);
        free(tname);
}

void open_index()
{
        char *basename = pparm_common_name("index");
        char *name     = NULL;
        uint iblock    = 0;
        uint psize;
        
        PPARM_INT_D(iblock, IBLOCK);
        PPARM_INT(psize, VM_PAGE_SIZE);
        
        asprintf(&name, "%s.%u", basename, iblock);
        open_index_i(name, psize);

        free(name);
}

void open_index_i(const char *name, uint psize)
{
        uint i, fd     = 0;

        page_size = psize;
        
        if ((fd = open(name, O_RDONLY | O_LARGEFILE, 0)) == -1)
                dub_sysdie("Couldn't open index %s", name);

        read(fd, (struct header_s*)&dex_header, sizeof(struct header_s));
        
        dex_base_size = dex_header.data_offs[0];
        dex_base_addr = mmap64(0, dex_base_size, PROT_READ, MAP_SHARED, fd, 0);
        if (dex_base_addr == MAP_FAILED)
                dub_sysdie("Couldn't map TOCs in %s", name);

        memset(dex_section, 0, DEX_NOF_SECT * sizeof(struct dex_section_s));
        
        for (i = 0; i < DEX_NOF_SECT; i++){
                dex_section[i].sect_fd   = fd;
                dex_section[i].sect_offs = dex_header.data_offs[i];
                dex_section[i].toc       = dex_base_addr + 
                                                dex_header.toc_offs[i];

                if (i == DEX_NOF_SECT - 1)
                        dex_section[i].nof_entries = 
                                (dex_header.data_offs[0]
                                - dex_header.toc_offs[i]) / sizeof(toc_e) - 1;
                else{
                        dex_section[i].nof_entries = 
                                (dex_header.toc_offs[i + 1] - 
                                 dex_header.toc_offs[i]) / sizeof(toc_e) - 1;
                }
        }
}

void close_index()
{
        _unmap_page(INFO);
        _unmap_page(FW);
        _unmap_page(INVA);
        _unmap_page(POS);
        munmap(dex_base_addr, dex_base_size);
}

void _mmap_page(enum dex_sect_id id, uint idx)
{
        struct dex_section_s *s = &dex_section[id];
        
        u64  offs     = 0;
        uint o_align  = 0;
        
        if (s->mmap_page)
                munmap(s->mmap_page, s->cur_size);

        if (idx > s->nof_entries)
                dub_die("Dex paging fault. Idx %u > %u.", idx, s->nof_entries);
        
        offs    = s->toc[idx].offs + s->sect_offs;
        o_align = offs & (getpagesize() - 1);
        
        s->first_item = idx;
        s->cur_offs   = s->toc[idx].offs;
        s->last_item  = find_last(s, idx);
        
        /* +4096 tries to make sure that our slack bit-twidling routines
         * in pcode handling can't cause a segmentation fault */
        s->cur_size = s->toc[s->last_item + 1].offs - s->cur_offs
                        + o_align + 4096;
       
        s->mmap_page = mmap64(0, s->cur_size, PROT_READ, MAP_SHARED,
                                 s->sect_fd, offs - o_align);
        
        if (s->mmap_page == MAP_FAILED)
                dub_sysdie("Couldn't mmap page (offs: %llu size: %llu)",
                                offs - o_align, s->cur_size);
        
        s->cur_start = s->mmap_page + o_align;
}

void _unmap_page(enum dex_sect_id id)
{
        if (dex_section[id].mmap_page){
                munmap(dex_section[id].mmap_page, dex_section[id].cur_size);
                dex_section[id].mmap_page  = NULL;
                dex_section[id].first_item = 0;
                dex_section[id].last_item  = 0;
        }
}

/* a simple binary search to find the last item which fits into the
 * page starting from the given idx. */
static inline uint find_last(const struct dex_section_s *s, uint first)
{       
        uint  left  = first;
        uint  right = s->nof_entries;
        
        u64   offs  = s->cur_offs + page_size;
        uint  idx   = 0;
        
        while (left <= right){
                
                u64 val = 0;
                
                idx = (left + right) >> 1;
                val = s->toc[idx].offs;
                
                if (val == offs)
                        return idx;
                else if (val > offs)
                        right = idx - 1;
                else
                        left = idx + 1;
        }

        return idx - 1;
}

static int inva_cmp(const void *i1, const void *i2)
{
        if (((toc_e*)i1)->val > ((toc_e*)i2)->val)
                return 1;
        else if (((toc_e*)i1)->val < ((toc_e*)i2)->val)
                return -1;
        return 0;
}

static const toc_e *toc_search(u32 key, const toc_e *start, int num)
{
        int  idx   = 0;
        int  left  = 0;
        int  right = num;
        
        while (left <= right){
                
                u64 val = 0;
                
                idx = (left + right) >> 1;
                val = start[idx].val;
                
                if (val == key)
                        return &start[idx];
                else if (val > key)
                        right = idx - 1;
                else
                        left = idx + 1;
        }

        return NULL;
}

int inva_find(u32 xid, const inva_e **data)
{       
        const toc_e *e = toc_search(xid, dex_section[INVA].toc,
                                   dex_section[INVA].nof_entries);
        /*
        toc_e *e = bsearch(&cmp, dex_section[INVA].toc, 
                   dex_section[INVA].nof_entries, sizeof(toc_e), inva_cmp);
        */

        if (!e){
                if (data)
                        *data = NULL;
                return -1;
        }

        int idx = e - dex_section[INVA].toc;
        
        if (data)
                *data = (const inva_e*)fetch_item(INVA, idx);

        return idx;
}

const const doc_e *info_find(u32 did)
{
        const toc_e *e = toc_search(did, dex_section[INFO].toc, DEX_NOF_SEG);
        return (const doc_e*)fetch_item(INFO, e - dex_section[INFO].toc);
}

const void *fw_find(u32 layer, u32 sid)
{
        int first = dex_header.fw_layers[layer].idx;

        /* empty layer */
        if (first >= dex_section[FW].nof_entries)
                return NULL;

        const toc_e *start = &dex_section[FW].toc[first];
        int num = dex_header.fw_layers[layer + 1].idx - 
                  dex_header.fw_layers[layer].idx;

        const toc_e *e = toc_search(sid, start, num);
        if (e)
                return fetch_item(FW, e - start + first);
        else
                return NULL;
}

const void *fw_fetch(u32 layer, u32 *sid, u32 *fw_iter)
{
        const toc_e *e = dex_section[FW].toc;
        u32 ssid = *sid;
        u32 max = dex_header.fw_layers[layer + 1].idx;
        u32 i = dex_header.fw_layers[layer].idx + (fw_iter ? *fw_iter: 0);

        for (;i < max; i++){
                if (e[i].val == ssid){
                        if (fw_iter)
                                *fw_iter = i - dex_header.fw_layers[layer].idx;
                        if (i + 1 < max){
                                *sid = e[i + 1].val;
                        }else
                                *sid = -1;
                        return fetch_item(FW, i);

                }else if (e[i].val > ssid){
                        if (fw_iter)
                                *fw_iter = i - dex_header.fw_layers[layer].idx;
                        *sid = e[i].val;
                        return NULL;
                }
        }
        *sid = -1;
        *fw_iter = i - dex_header.fw_layers[layer].idx;
        return NULL;
}

const u32 *decode_segment(u32 sid, uint segment_size)
{         
        static u32 *doc;

        if (!doc)
                doc = xmalloc(segment_size * 4);    
        memset(doc, 0, segment_size * 4);

        const void *xidlst = fetch_item(POS, sid);
        const void *poslst = &xidlst[dex_section[POS].toc[sid].val];

        u32 xid, poffs;
        u32 offs = xid = poffs = 0;
        u32 tlen = elias_gamma_read(xidlst, &offs) - 1;
        if (!tlen)
                return NULL;

        u32 rice_f1 = rice_read(xidlst, &offs, 2);
        u32 rice_f2 = rice_read(xidlst, &offs, 2);

        while (tlen--){
                xid += rice_read(xidlst, &offs, rice_f1);
                poffs += rice_read(xidlst, &offs, rice_f2);

                u32 pos;
                u32 tmp = poffs;
                DEX_FOREACH_VAL(poslst, &tmp, pos)
                        /*
                        if (pos - 1 >= segment_size)
                                dub_die("pos %u larger than segment size %u",
                                        pos - 1, segment_size);
                        */
                        doc[pos - 1] = xid;
                DEX_FOREACH_VAL_END
        }

        return doc;
}

void dex_stats()
{
       uint i = 0;
       for (i = 0; i < DEX_NOF_SECT; i++)
               dub_msg("Section %s: Fetches %u faults %u.", 
                               dex_names[i], dex_section[i].fetches,
                               dex_section[i].fetch_faults);
}
