/*
 *   preproc/encode_fwlayers.c
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
#include <string.h>

#include <dub.h>
#include <pparam.h>
#include <gglist.h>
//#include <pcode.h>
#include <rrice.h>
#include <dexvm.h>
#include <mmisc.h>

#include <Judy.h>

#define IO_BUF_SIZE 1048576 /* 1MB */ 

#define LAYERS "1 10 50 100 1000 5000 10000 20000 50000"

static FILE *data_f;
static FILE *toc_f;

static layer_nfo fw_layers[NUM_FW_LAYERS + 1];

static Pvoid_t ixemes_freq_range(int p, int min_f, int max_f)
{
        Pvoid_t ix = NULL;
        Word_t xid;
        int i, tst;
        for (i = 0; i < dex_section[INVA].nof_entries; i++){

                xid = dex_section[INVA].toc[i].val;
                
                if (xid >= XID_META_FREQUENT_F &&
                    xid <= XID_META_FREQUENT_L)
                        continue;
                
                if (xid <= XID_TOKEN_FREQUENT_L &&
                    xid >= XID_TOKEN_FREQUENT_F)
                        continue;

                const inva_e *e = 
                        (const inva_e*)fetch_item(INVA, i);
                if (e->len > min_f && e->len <= max_f){
                        J1S(tst, ix, xid);
                }
        }

        xid = 0;
        J1F(tst, ix, xid);
        fw_layers[p].min_xid = xid;
        xid = -1;
        J1L(tst, ix, xid);
        fw_layers[p].max_xid = xid;
        
        J1C(tst, ix, 0, -1);
        dub_msg("Layer %u: number of xids %u min %u max %u", p, tst,
                fw_layers[p].min_xid, fw_layers[p].max_xid);

        return ix;
}

static inline void encode_segment(const glist *g)
{
        static char *buf;
        static uint buf_len;

        u32 offs = 0;
        uint need = BITS_TO_BYTES(rice_encode(NULL, &offs, g->lst, g->len));

        if (need > buf_len){
                if (buf)
                        free(buf);
                buf_len = need;
                buf = xmalloc(buf_len);
                memset(buf, 0, buf_len);
        }
        offs = 0;
        rice_encode(buf, &offs, g->lst, g->len);
        offs = BITS_TO_BYTES(offs);

        fwrite(buf, offs, 1, data_f);
        memset(buf, 0, offs);
}

static int encode_layer(const Pvoid_t ix)
{
        uint sid, no = 0;
        growing_glist *gg;
        GGLIST_INIT(gg, 100);

        for (sid = 0; sid < dex_section[FW].nof_entries; sid++){
                const void *d   = fetch_item(FW, sid);
                u32        offs = 0;
                u32        xid, tst;
                
                gg->lst.len = 0;
                DEX_FOREACH_VAL(d, &offs, xid)
                        J1T(tst, ix, xid);
                        if (tst){
                                GGLIST_APPEND(gg, xid);
                        }
                DEX_FOREACH_VAL_END
                
                if (gg->lst.len){
                        toc_e toc;
                        toc.val = sid; //dex_section[FW].toc[i].val;
                        toc.offs = ftello64(data_f);
                        fwrite(&toc, sizeof(toc_e), 1, toc_f);
                        
                        int i;
                        for (i = 0; i < gg->lst.len; i++){
                                if (!gg->lst.lst[i])
                                        dub_die("FOUND");
                        }

                        encode_segment(&gg->lst);
                        ++no;
                }
        }

        free(gg);
        dub_msg("%u / %u sids matched", no, dex_section[FW].nof_entries);
        return no;
}

int main(int argc, char **argv)
{
        char    *basename = NULL;
        char    *tname    = NULL;
        char    *dname    = NULL;
        char    *pname    = NULL;
        char    *layers_str;

        FILE *part_f;

        uint iblock = 0;

        dub_init();
        
        PPARM_INT_D(iblock, IBLOCK);
        PPARM_STR(layers_str, LAYERS);
        glist *layers = str2glist_len(&layers_str, NUM_FW_LAYERS - 1);

        open_section(FW, iblock);
        open_section(INVA, iblock);

        basename = pparm_common_name("fwlayers");
        asprintf(&dname, "%s.%u.sect", basename, iblock);
        asprintf(&tname, "%s.%u.toc.sect", basename, iblock);
        asprintf(&pname, "%s.%u.nfo", basename, iblock);
        free(basename);
         
        if (!(data_f = fopen64(dname, "w+")))
                dub_sysdie("Couldn't open section %s", dname);
        if (!(toc_f = fopen64(tname, "w+")))
                dub_sysdie("Couldn't open section %s", tname);
        if (!(part_f = fopen64(pname, "w+")))
                dub_sysdie("Couldn't open section %s", tname);
        
        if (setvbuf(data_f, NULL, _IOFBF, IO_BUF_SIZE))
                dub_sysdie("Couldn't open input buffer");
        if (setvbuf(toc_f, NULL, _IOFBF, IO_BUF_SIZE))
                dub_sysdie("Couldn't open input buffer");

        u32 num_entries = 0;
        fw_layers[0].idx = num_entries;

        int p, no, prev_p = 0, prev_offs = 0;
        for (p = 0; p < NUM_FW_LAYERS - 1; p++){
                
                dub_msg("Layer %u", p);
                Pvoid_t ix = ixemes_freq_range(p, prev_p, layers->lst[p]);
                prev_p = layers->lst[p];
               
                num_entries += encode_layer(ix);
                dub_msg("LAYER %u NUM %u", p, num_entries);
                fw_layers[p + 1].idx = num_entries;
                fw_layers[p + 1].max_freq = layers->lst[p];

                u32 offs = (u32)ftello64(data_f);
                dub_msg("Layer takes %u bytes", offs - prev_offs);
                prev_offs = offs;

                J1FA(no, ix);
        }
        Pvoid_t ix = ixemes_freq_range(p, prev_p, 1 << 31);
        num_entries += encode_layer(ix);
        fw_layers[p + 1].idx = num_entries;
        fw_layers[p + 1].max_freq = 1 << 31;
        
        u32 offs = (u32)ftello64(data_f);
        dub_msg("Layer takes %u bytes", offs - prev_offs);

        fwrite(fw_layers, sizeof(fw_layers), 1, part_f);

        return 0;
}
