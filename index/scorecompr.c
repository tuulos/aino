/*
 *   index/scorecompr.c
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

#include <math.h>
#include <Judy.h>

#include <rrice.h>
#include <scorelib.h>
#include <dexvm.h>
//#include <pcode.h>

#define EPS 0.0000001
#define LINK_PENALTY 32


static inline uint encode_seg(char *dst, u32 *offs,
                        const struct scored_ix *sco, 
                        uint x0, uint x1, s32 *lstbuf,
                        uint *cost, uint fac)
{
        float a = -(sco[x0].score - sco[x1].score) / (x1 - x0);
        float b = sco[x0].score;
        int i = 0;
        uint x = 0;

        while (x0 + x <= x1){
                lstbuf[i++] = fac * (sco[x0 + x].score - (a * x + b));
                //if (dst)
                        //printf("KAS[%u] %f\n", x0 + x, sco[x0 + x].score);
                ++x;
        }
        *cost = rice_encode_signed(dst, offs, lstbuf, i);

        return x0 + x;
}

/*
 * lincompr_scores approximates score distribution ixscores
 * by line segments. Segments are found in breadth-first manner,
 * dividing x-axis to 2, 4, 8... non-overlapping segments (outermost
 * while-loop). Trivially there are log(len) possible segmentations.
 *
 * Line segments starts at (x0, ixscores[x0].score) and ends at
 * (x1, ixscores[x1].score). Residuals are encoded using Rice
 * coding (encode_seg()). If the code length for level z segmentation 
 * is higher than for level z - 1, we stick to level z - 1 segmentation
 * and evaluate no more segments within that range (fixed-array).
 *
 * At most len values are encoded within a segmentation level, thus
 * the algorithm requires O(len * log(len)) steps.
 * 
 * lincompr_scores expects that ixscore.scores are monotonically
 * decreasing (no duplicate scores).
 *
 * |----------------------------|
 * |             |<- pivot      | z = 1
 * |----------------------------|          segmentation
 * |      |      |      |       | z = 2       level
 * |----------------------------|
 * |  |   |   |  |   |  |   |   | z = 3
 * ------------------------------
 * 0 <-------- X-axis ----------> len 
 *
 */
void *lincompr_scores(struct scored_ix *ixscores, int len, 
                        uint *cost, uint fac)
{
        uint i, x, p, z, covered;

        struct pivot{
                uint x;
                uint score;
        };
        
        struct pivot *pivots =
                xmalloc(sizeof(struct pivot) * (len / 2 + 1));
        struct pivot *new_pivots = 
                xmalloc(sizeof(struct pivot) * (len / 2 + 1));
        s32 *lstbuf = xmalloc((len / 2 + 1) * 4);

        Pvoid_t fixed = NULL;

        z = covered = *cost = 0;
        pivots[0].x = len - 1;
        pivots[0].score = INT_MAX;

        /* loop until the whole x-axis is covered by line segments */
        while (covered < len){
                i = p = x = 0;
                ++z;

                while (x < len){
                        Word_t *ptr;
                        uint sco_l, sco_r, x0, x1, xs, end_cond = 0;
                        uint offs = 0;

                        /* segment [x..] already fixed? */
                        JLG(ptr, fixed, x);
                        if (ptr){
                                x = *ptr + 1;
                                p = 0;
                                while (pivots[p].x < x)
                                        ++p;
                                continue;
                        }

                        /* left segment */
                        x0 = xs = x;
                        x1 = x + (pivots[p].x - x) / 2;
                        if (x1 - x0 <= 2)
                                end_cond = 1;
                       
                        //printf("L (%d, %d)\n", x0 , x1);

                        encode_seg(NULL, &offs, ixscores, 
                                x0, x1, lstbuf, &sco_l, fac);
                        
                        new_pivots[i].x = x1;
                        new_pivots[i++].score = sco_l;
                        
                        /* right segment */
                        x0 = x1;
                        x1 = pivots[p].x;
                        if (x1 - x0 <= 2)
                                end_cond = 1;
                        
                        //printf("R (%d, %d)\n", x0 , x1);
                        
                        encode_seg(NULL, &offs, ixscores, x0, x1, 
                                lstbuf, &sco_r, fac);
                        
                        new_pivots[i].x = x1;
                        new_pivots[i++].score = sco_r;

                        /* splitting doesn't improve the score? */
                        if (end_cond || offs + z * LINK_PENALTY > 
                                pivots[p].score + (z - 1) * LINK_PENALTY){
                               
                                //printf("SEG (%d,%d)\n", xs, x1);

                                JLI(ptr, fixed, xs);
                                *ptr = x1;
                                covered += (x1 + 1) - xs;
                                *cost += pivots[p].score;
                        }
                        ++p;
                        x = x1 + 1;
                }

                new_pivots[i++].x = INT_MAX;

                struct pivot *tmp = pivots;
                pivots = new_pivots;
                new_pivots = tmp;
        }
       
        free(pivots);
        free(new_pivots);
        free(lstbuf);

        return fixed;
}

struct rank_ix{
        u32 xid;
        uint rank;
};

static int rank_cmp(const void *i1, const void *i2)
{
        uint xid1 = ((const struct rank_ix*)i1)->xid;
        uint xid2 = ((const struct rank_ix*)i2)->xid;

        if (xid1 < xid2)
                return 1;
        else if (xid1 > xid2)
                return -1;

        return 0;
}

#define XID_BLOCK 32

static uint encode_xids(char *dest, u32 *offs, const u32 *xids, uint xids_len)
{
        int j, i = 0;

        uint orig_offs = *offs;
        struct rank_ix *block = xmalloc(XID_BLOCK * sizeof(struct rank_ix));
        s32 *dis = xmalloc(XID_BLOCK * 4);

        elias_gamma_write(dest, offs, xids_len);

        while (i < xids_len){

                int q = (i + XID_BLOCK < xids_len ? XID_BLOCK: xids_len - i);
                for (j = 0; j < q; j++){
                        block[j].xid = xids[i + j];
                        block[j].rank = j;
                }
                qsort(block, q, sizeof(struct rank_ix), rank_cmp);
                int all_zero = 1;
                for (j = 0; j < q; j++){
                        dis[j] = block[j].rank - j;
                        if (dis[j])
                                all_zero = 0;
                }
                if (all_zero)
                        rice_encode_signed(dest, offs, dis, 0);
                else
                        rice_encode_signed(dest, offs, dis, q);
                
                elias_gamma_write(dest, offs, block[0].xid);
                for (j = 1; j < q; j++)
                        elias_gamma_write(dest, offs, 
                                block[j - 1].xid - block[j].xid);
                i += q;
        }
        
        /*
        printf("XIDS: ");
        for (i = 0; i < xids_len; i++)
                printf("%u ", xids[i]);
        printf("\n");
        */
        
        free(block);
        free(dis);

        return *offs - orig_offs;
}

static struct scored_ix *decode_xids(const char *src, u32 *offs, uint *len)
{
        s32 *dest = NULL;
        uint dest_len = 0;

        *len = elias_gamma_read(src, offs);
        struct scored_ix *ix = xmalloc(*len * sizeof(struct scored_ix));
        int j, i = 0;
        while (i < *len){

                int dp_len = rice_decode_signed(&dest, &dest_len, src, offs);
                int q = (i + XID_BLOCK < *len ? i + XID_BLOCK: *len);

                if (dp_len){
                        /* displacement list non-empty:
                         * we should reorder the xids */
                        u32 prev_xid;
                        ix[i + dest[0]].xid = prev_xid = 
                                elias_gamma_read(src, offs);
                        for (j = 1; j < dp_len; j++){
                                prev_xid -= elias_gamma_read(src, offs);
                                ix[j + dest[j] + i].xid = prev_xid;
                        }
                        i += j;
                }else{
                        /* ixemes already in order */
                        ix[i++].xid = elias_gamma_read(src, offs);
                        for (; i < q; i++)
                                ix[i].xid = ix[i - 1].xid - 
                                        elias_gamma_read(src, offs);
                }
        }
        /*
        printf("XIDS: ");
        for (i = 0; i < *len; i++)
                printf("%u ", ix[i].xid);
        printf("\n");
        */
        free(dest);
        return ix;
}

static char *pack_scores(uint fac, const u32 *xids, uint xids_len, 
        const u32 *counts, uint counts_len, const Pvoid_t segs, uint segs_cost,
        const struct scored_ix *ixscores, uint *buf_len)
{
        uint offs = 0;
        uint nof_segs;
        JLC(nof_segs, segs, 0, -1);

        uint sze = 32 + nof_segs * 64 + segs_cost +
                   encode_xids(NULL, &offs, xids, xids_len) +
                   rice_encode(NULL, &offs, counts, counts_len) + 64;

        char *p;
        char *buf = p = xmalloc(sze / 8 + 1);
        memset(buf, 0, sze / 8 + 1);

        memcpy(p, &fac, sizeof(uint));
        p += sizeof(uint);

        offs = 0;

        encode_xids(p, &offs, xids, xids_len);
        rice_encode(p, &offs, counts, counts_len);
        
        p += offs / 8;
        if (offs & 7)
                ++p;
       
        memcpy(p, &nof_segs, sizeof(uint));
        p += sizeof(uint);

        Word_t idx = 0;
        Word_t *ptr;
        JLF(ptr, segs, idx);
        while (ptr){
                memcpy(p, &ixscores[idx].score, sizeof(float));
                p += 4;
                memcpy(p, &ixscores[*ptr].score, sizeof(float));
                p += 4;
                JLN(ptr, segs, idx);
        }
        
        if (nof_segs > 1){
                s32 *lstbuf = xmalloc((xids_len / 2 + 1) * 4);

                idx = offs = 0;
                JLF(ptr, segs, idx);
                while (ptr){
                        uint tmp;
                        encode_seg(p, &offs, ixscores, idx, *ptr, 
                                lstbuf, &tmp, fac);
                        JLN(ptr, segs, idx);
                }
                
                p += offs / 8;
                if (offs & 7)
                        ++p;
        
                free(lstbuf);
        }

        *buf_len = p - buf;
        return buf;
}

/* assumes that ixscores is ordered primarily by descending scores and 
 * secondly by descending ixemes */
char *compress_scores(const struct scored_ix *ixscores, uint ix_len,
        uint *buf_len, uint fac)
{
        int i, j, c;

        u32 *xids = xmalloc(ix_len * 4);
        struct scored_ix *uniq = xmalloc(ix_len * sizeof(struct scored_ix));

        /* count and get rid of duplicates */
        u32 *counts = xmalloc(ix_len * 4);
        xids[0] = ixscores[0].xid;
        uniq[0] = ixscores[0];

        for (c = 0, j = 1, i = 1; i < ix_len; i++){
                xids[i] = ixscores[i].xid;
                if (ixscores[i].score == ixscores[i - 1].score)
                        ++c;
                else{
                        counts[j - 1] = c + 1;
                        c = 0;
        
                        uniq[j].xid = ixscores[i].xid;
                        uniq[j].score = ixscores[i].score;
                        ++j;
                }
        }
        
        counts[j - 1] = c + 1;

        uint segs_cost = 0;
        Pvoid_t segs = NULL;
        if (j > 2)
                segs = lincompr_scores(uniq, j, &segs_cost, fac);
        else{
                Word_t *ptr;
                Word_t idx = 0;
                JLI(ptr, segs, idx);
                *ptr = j - 1;
        }

        char *ret = pack_scores(fac, xids, ix_len, counts, j, 
                segs, segs_cost, uniq, buf_len);

        JLFA(j, segs);
        free(uniq);
        free(xids);
        free(counts);

        return ret;
}

struct scored_ix *uncompress_scores(const char *buf, uint buf_len, 
        uint *ix_len)
{
        uint nof_segs, i, j, fac, offs = 0;
        memcpy(&fac, buf, sizeof(uint));
        buf += sizeof(uint);

        s32 *dest = NULL;
        uint dest_len = 0;
        
        struct scored_ix *ixsco = decode_xids(buf, &offs, ix_len);
        
        uint count_len = rice_decode((u32**)&dest, &dest_len, buf, &offs);
        u32 *counts = xmalloc(count_len * 4);
        memcpy(counts, dest, count_len * 4);

        buf += offs / 8;
        if (offs & 7)
                ++buf;
        
        memcpy(&nof_segs, buf, sizeof(uint));
        buf += sizeof(uint);
        
        float *segs = xmalloc(nof_segs * 2 * sizeof(float));
        for (i = 0; i < nof_segs * 2; i += 2){
                memcpy(&segs[i], buf, sizeof(uint));
                buf += sizeof(float);
                memcpy(&segs[i + 1], buf, sizeof(uint));
                buf += sizeof(float);
        }
                                
        
        if (nof_segs > 1){
                uint x0 = 0;
                for (offs = 0, j = 0, i = 0; i < nof_segs * 2; i += 2){
                        
                        uint no = rice_decode_signed(&dest, 
                                &dest_len, buf, &offs);
        
                        uint x = 0;
                        uint x1 = x0 + no - 1;
                        float a = -(segs[i] - segs[i + 1]) / (x1 - x0);
                        float b = segs[i];

                        while (x0 + x <= x1){
                                float sco = (dest[x] / (float)fac) + a * x + b;
                                int k;
                                for (k = 0; k < counts[x0 + x]; k++)
                                        //printf("SCO[%u] = %f\n", j, sco);
                                        ixsco[j++].score = sco;
                                ++x;
                        }

                        x0 += x;
                }
        }else{
                int k;
                for (k = 0; k < counts[0]; k++)
                        ixsco[k].score = segs[0];
                ixsco[*ix_len - 1].score = segs[1];
        }

        free(segs);
        free(counts);
        free(dest);

        return ixsco;
}


float comp_error(const struct scored_ix *orig, 
        const struct scored_ix *est, uint len)
{
        int i;
        float err = 0;
        for (i = 0; i < len; i++){
                if (orig[i].xid != est[i].xid){
                        printf("ERR XID[%u] %u != %u\n", i, 
                                orig[i].xid, est[i].xid);
                        return -1;
                }
                err += fabs(orig[i].score - est[i].score);
        }

        return err;
}

#if 0

#include <pparam.h>
#include <finnuio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
        dub_init();

        const char *fname = pparm_common_name("ixi");
        
        load_ixicon(fname);
        open_index();

        struct xid_item{
                u32 xid;
                u32 rank;
        };

        glist *cues = xmalloc(4 + sizeof(glist));
        cues->len = 1;
        
        Word_t xid = get_xid(argv[1]);
        cues->lst[0] = xid;

        Pvoid_t scores = scoretable(cues);
        int len, j, c, k, i = 0;
        JLC(len, scores, 0, -1);
        struct scored_ix *ixscores = xmalloc(len * sizeof(struct scored_ix));
        struct scored_ix *ixscores2 = xmalloc(len * sizeof(struct scored_ix));
        
        uint olen = len;

        Word_t *ptr;
        JLF(ptr, scores, xid);
        while (ptr){
                const inva_e *e;
                inva_find(xid, &e);
                float sco = *ptr / (float)e->len;
                if (sco > EPS){
                        ixscores[i].score = sco;
                        ixscores[i].xid = xid;
                        ++i;
                }

                JLN(ptr, scores, xid);
        }
        len = i;
       
        qsort(ixscores, len, sizeof(struct scored_ix), ixscore_cmp);
        memcpy(ixscores2, ixscores, len * sizeof(struct scored_ix));

        uint buf_len;
        char *pack_buf = compress_scores(ixscores2, len, &buf_len, 1e3);
        struct scored_ix *newsco = uncompress_scores(pack_buf, buf_len);
        float err = comp_error(ixscores, newsco, len);
        
        printf("W '%s' bytes %u err %f\n", argv[1], buf_len, err);
        
        return 0;
}

#endif
