/*
 *   indexing/dexvm.h
 *   mmaps parts of index to memory
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

#ifndef __DEXVM_H__
#define __DEXVM_H__

#include <ttypes.h>
#include <ixemes.h>
#include <dub.h>

#define DEX_NOF_SECT 4
enum dex_sect_id {INFO=0, FW=1, INVA=2, POS=3};


#define NUM_FW_LAYERS 10
#define LOW_LAYER_LIMIT 3
#define MED_LAYER_LIMIT 6
#define HIGH_LAYER_LIMIT 10

//#define NUM_FW_PARTS (sizeof(FW_PARTITION) / sizeof(int) + 1)
//#define PART_SID(part, sid) (part * DEX_NOF_SEG + sid)

static const char *dex_names[] __attribute__((unused)) = 
        {"info", "fwlayers", "inva", "pos"};

static const char *pre_dex_names[] __attribute__((unused)) = 
        {"info", "fw", "inva", "pos"};

typedef struct{
        u32 idx;
        u32 max_freq;
        u32 min_xid;
        u32 max_xid;
} layer_nfo;

struct header_s{
        u32   index_id;
        u32   iblock;
        u32   segment_size;
        layer_nfo fw_layers[NUM_FW_LAYERS + 1];
        /* TOCS */
        u64 toc_offs[DEX_NOF_SECT];
        /* DATA */
        u64 data_offs[DEX_NOF_SECT];
} __attribute__((packed));

typedef struct{
        u32 offs;
        u32 val;
} toc_e;

typedef struct{
        u32 len;
        const char inva[0];
} inva_e;

typedef struct{
        u32 key;
        u32 first_sid;
        float prior;
} doc_e;

struct dex_section_s{

        /* Section location */
        int        sect_fd;
        u64        sect_offs;
        
        /* TOC for items of this section */
        const toc_e *toc;
        u32         nof_entries;
        
        /* Currently mapped page */
        u32         first_item;  /* first available item */
        u32         last_item;   /* last available item */
        void        *mmap_page;  /* page start */
        const void  *cur_start;  /* first item within the page */
        u64         cur_size;    /* page size */
        u32         cur_offs;    /* page offset w.r.t TOC */

        /* Statistics */
        uint        fetches;
        uint        fetch_faults;
};

extern struct dex_section_s dex_section[DEX_NOF_SECT];
extern const struct header_s dex_header;

/* Some aliases */
#define DEX_NOF_IX   dex_section[INVA].nof_entries
#define DEX_NOF_SEG  dex_section[INFO].nof_entries
#define DEX_NOF_DOX  (dex_section[INFO].toc[DEX_NOF_SEG - 1].val + 1)
#define DID2SID(did) info_find(did)->first_sid
#define SID2DID(sid) dex_section[INFO].toc[sid].val
#define DID2KEY(did) info_find(did)->key
#define IDX2XID(idx) dex_section[INVA].toc[idx].val

#define DOCPRIOR(sid) ((const doc_e*)fetch_item(INFO, sid))->prior
#define DOC_FIRST_SID(sid) ((const doc_e*)fetch_item(INFO, sid))->first_sid

void open_section(enum dex_sect_id id, uint iblock_no);
void open_section_i(const char *basename, 
        enum dex_sect_id sect, uint iblock_no);
void open_index();
void open_index_i(const char *name, uint psize);
void close_index();
int inva_find(u32 xid, const inva_e **data);
const doc_e *info_find(u32 did);
const void *fw_find(u32 layer, u32 sid);
const void *fw_fetch(u32 layer, u32 *sid, u32 *fw_iter);
const u32 *decode_segment(u32 sid, uint segment_size);
void dex_stats();

/* for internal use */
void _mmap_page(enum dex_sect_id id, uint idx);
void _unmap_page(enum dex_sect_id id);

inline static const void *fetch_range(enum dex_sect_id id, 
                                      uint first_idx, uint last_idx)
{
        if (first_idx < dex_section[id].first_item ||
            last_idx >= dex_section[id].last_item){
                _mmap_page(id, first_idx);
                ++dex_section[id].fetch_faults;
                /* XXX Hmm. Is this guaranteed to work if 
                 * last_idx == dex_section[id].nof_entries - 1? */
                if (last_idx > dex_section[id].last_item)
                        dub_die("VM_PAGE_SIZE too small: IDX range %u-%u "
                                "does not fit into one page", 
                                first_idx, last_idx);
        }
        ++dex_section[id].fetches;
        
        return dex_section[id].cur_start +
               dex_section[id].toc[first_idx].offs - 
               dex_section[id].cur_offs;
}

inline static const void *fetch_item(enum dex_sect_id id, uint idx)
{
        if (idx < dex_section[id].first_item ||
            idx >= dex_section[id].last_item){
                _mmap_page(id, idx);
                ++dex_section[id].fetch_faults;
        }

        ++dex_section[id].fetches;

        return dex_section[id].cur_start +
               dex_section[id].toc[idx].offs - 
               dex_section[id].cur_offs;
}

#endif /* __DEXVM_H__ */
