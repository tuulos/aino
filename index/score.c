/*
 *   index/score.c
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

#include <string.h>
#include <math.h>
#include <time.h>

#include <qexpansion.h>
#include <gglist.h>
#include <rrice.h>
#include <pparam.h>

#include "dexvm.h"
#include "scorelib.h"

#include <Judy.h>

#define TIMER_DEF clock_t __start;
#define TIMER_START __start = clock();
#define TIMER_END(msg) dub_msg(msg,\
        ((double) (clock() - __start)) / (CLOCKS_PER_SEC / 1000.0));

int ixscore_cmp(const void *i1, const void *i2)
{
        float score1 = ((const struct scored_ix*)i1)->score;
        float score2 = ((const struct scored_ix*)i2)->score;
        
        if (score1 < score2)
                return 1;
        else if (score1 > score2)
                return -1;

        uint xid1 = ((const struct scored_ix*)i1)->xid;
        uint xid2 = ((const struct scored_ix*)i2)->xid;

        if (xid1 < xid2)
                return -1;
        else if (xid1 > xid2)
                return 1;

        return 0;
}

#if 0
static u32 isect_cueset(const inva_e *e, const Pvoid_t cueset)
{
        u32 offs = 0;
        u32 occ = 0;
        Word_t sid = 0;
        int tst;

        J1F(tst, cueset, sid);
        u32 first_sid = sid;
        sid = -1;
        J1L(tst, cueset, sid);
        u32 last_sid = sid;
        
        /*
        INVA_FOREACH_VAL(e, &offs, sid)
                if (sid >= first_sid)
                        break;
        INVA_FOREACH_VAL_END
        
        J1T(tst, cueset, sid);
        occ += tst;
*/
        INVA_FOREACH_VAL(e, &offs, sid)
                J1T(tst, cueset, sid);
                occ += tst;
                /*
                if (sid >= last_sid)
                        break;
                        */
        INVA_FOREACH_VAL_END

        return occ;
}

Pvoid_t score_part_some(uint part, const Pvoid_t segs, const Pvoid_t cueset)
{
        Pvoid_t scores = NULL;
        Pvoid_t xids = NULL;
        int tst;
        Word_t *ptr;
        u32 fw_iter = 0;

        /* 1. Find out which ixemes occur in segs */
        
        TIMER_DEF
        TIMER_START
        Word_t sid = 0;
        J1F(tst, segs, sid);
        while(tst){
                u32 offs = 0;
                u32 xid;
                //const void *p = fw_fetch(part, (u32*)&sid, &fw_iter);
                const void *p = fw_find(part, sid);
                if (p){
                        DEX_FOREACH_VAL(p, &offs, xid)
                                J1S(tst, xids, xid);
                        DEX_FOREACH_VAL_END
                }
                //J1F(tst, segs, sid);
                J1N(tst, segs, sid);
        }
        Word_t cueset_len;
        JLC(cueset_len, xids, 0, -1);
        dub_msg("DIDI(%u) %u", part, cueset_len);
        
        TIMER_END("Finding out xidset took %gms")
        TIMER_START

        /* 2. Score the found ixemes against the cueset */

        Word_t xid = 0;
        J1F(tst, xids, xid);
        while (tst){               
                const inva_e *e;
                inva_find(xid, &e);
                if (e){
                        int occ = isect_cueset(e, cueset);
                        if (occ){
                                JLI(ptr, scores, xid);
                                *ptr = occ;
                                //float sco = occ / (float)e->len;
                                //memcpy(ptr, &sco, sizeof(float));
                        }else
                                dub_die("eh %u", xid);
                }

                J1N(tst, xids, xid);
        }
        
        TIMER_END("Xidset took %gms")

        J1FA(tst, xids);
        return scores;
}
#endif

#define SCORETABLE_SIZE 50000

Pvoid_t layer_freq_high(Pvoid_t scores, uint layer, const glist *cueset)
{
        static u32 freq[SCORETABLE_SIZE];
        /*
        if (!freq)
                freq = xmalloc(SCORETABLE_SIZE * 4);
        */
        memset(freq, 0, SCORETABLE_SIZE * 4);

        u32 i, min_xid = dex_header.fw_layers[layer].min_xid;
        Word_t *ptr;
        
        //J1F(tst, cueset, sid);
        //while (tst){
        for (i = 0; i < cueset->len; i++){
                u32 offs = 0;
                u32 xid;
                //const void *p = fw_fetch(layer, (u32*)&sid, &fw_iter);
                const void *p = fw_find(layer, cueset->lst[i]);
                if (p){
                        DEX_FOREACH_VAL(p, &offs, xid)
                                if (xid - min_xid < SCORETABLE_SIZE)
                                        ++freq[xid - min_xid];
                                else{
                                        JLI(ptr, scores, xid);
                                        ++*ptr;
                                }
                        DEX_FOREACH_VAL_END
                }
                //J1F(tst, cueset, sid);
                //J1N(tst, cueset, sid);
        }
        for (i = 0; i < SCORETABLE_SIZE; i++){
                if (freq[i]){
                        Word_t xid = i + min_xid;
                        JLI(ptr, scores, xid);
                        *ptr += freq[i];
                }
        }
        return scores;
}

#if 0
Pvoid_t layer_freq(Pvoid_t scores, u32 layer, const Pvoid_t cueset)
{
        //Pvoid_t scores = NULL;
        Word_t tst, sid = 0;
        Word_t *ptr;
        u32 fw_iter = 0;
        J1F(tst, cueset, sid);
        while (tst){
                u32 offs = 0;
                u32 xid;
                //const void *p = fw_fetch(layer, (u32*)&sid, &fw_iter);
                const void *p = fw_find(layer, sid);
                if (p){
                        DEX_FOREACH_VAL(p, &offs, xid)
                                JLI(ptr, scores, xid);
                                ++*ptr;
                        DEX_FOREACH_VAL_END
                }
                //J1F(tst, cueset, sid);
                J1N(tst, cueset, sid);
        }
        return scores;
} 


Pvoid_t normalize_scores(Pvoid_t scores)
{
        Word_t *ptr;
        Word_t xid = 0;
        JLF(ptr, scores, xid);
        while (ptr){
                float sco = *ptr;
                const inva_e *e;
                inva_find(xid, &e);
                sco /= e->len;
                memcpy(ptr, &sco, sizeof(float));
                JLN(ptr, scores, xid);
        }
        return scores;
}


Pvoid_t layer_ixscores(uint layer, const Pvoid_t cueset, 
                       const Pvoid_t acc_scores)
{

        /* 1. Collect ixeme occurrences within the queset */

        Pvoid_t scores;
        if (layer < LOW_LAYER_LIMIT)
                scores = layer_freq(NULL, layer, cueset);
        else
                scores = layer_freq_high(NULL, layer, cueset);
       
        int tst;
        JLC(tst, scores, 0, -1);
        dub_msg("Layer %u all: Found %u xids", layer, tst);
        
        /* 2. Normalize scores */

        Word_t *ptr;
        Word_t xid = 0;
        JLF(ptr, scores, xid);
        while (ptr){
                float sco = *ptr;
                Word_t *aptr;
                JLG(aptr, acc_scores, xid);
                if (aptr) 
                        sco += *aptr;
                const inva_e *e;
                inva_find(xid, &e);
                if (!e)
                        dub_die("XID %u missing", xid);
                sco /= e->len;
                memcpy(ptr, &sco, sizeof(float));
                JLN(ptr, scores, xid);
        }
        return scores;
}
#endif

#define NUM_BUCKETS 1000

/* bucket sort */
struct scored_ix *sort_scores(const Pvoid_t scores)
{
        Word_t num_scores, xid, i, j, k;
        JLC(num_scores, scores, 0, -1);

        struct scored_ix *ixscores = xmalloc(num_scores * 
                sizeof(struct scored_ix));

        u32 *sizes = xmalloc(NUM_BUCKETS * 4);
        memset(sizes, 0, NUM_BUCKETS * 4);
        struct scored_ix **buckets = xmalloc(NUM_BUCKETS * sizeof(void*));
        memset(buckets, 0, NUM_BUCKETS * sizeof(void*));

        xid = 0;
        Word_t *ptr;
        JLF(ptr, scores, xid);
        while (ptr){
                int idx = (NUM_BUCKETS - 1) * (*(float*)ptr);
                if (idx > NUM_BUCKETS - 1){
                        dub_warn("BUG: In sort_scores: XID %u has score %f "
                                 "> 1.0 which is not good. Check this query.",
                                        xid, (*(float*)ptr));
                        idx = NUM_BUCKETS - 1;
                }
                if (!(sizes[idx] & 127)){
                        buckets[idx] = realloc(buckets[idx], 
                                (128 + sizes[idx]) * sizeof(struct scored_ix));
                        memset(&buckets[idx][sizes[idx]], 0, 
                                (128 * sizeof(struct scored_ix)));
                }
                buckets[idx][sizes[idx]].xid = xid;
                buckets[idx][sizes[idx]].score = *(float*)ptr;
                ++sizes[idx];
                JLN(ptr, scores, xid);
        }

        k = 0;
        i = NUM_BUCKETS;
        while (i--){
                if (!sizes[i])
                        continue;
                
                float prev = buckets[i][0].score;
                for (j = 1; j < sizes[i]; j++)
                        if (fabs(buckets[i][j].score - prev) > 0.00001){
                                qsort(buckets[i], sizes[i], 
                                        sizeof(struct scored_ix), ixscore_cmp);
                                break;
                        }

                for (j = 0; j < sizes[i]; j++)        
                        ixscores[k++] = buckets[i][j];
                free(buckets[i]);
        }

        free(sizes);
        free(buckets);
        
        return ixscores;
}

Pvoid_t expand_cueset(const glist *cues)
{
        Pvoid_t queset = NULL;
        Word_t i, j;

        void add_to_queset(u32 xid)
        {
                const inva_e *e;
                u32 offs = 0;
                u32 sid;
                int tst;

                if (inva_find(xid, &e) == -1)
                        return;

                INVA_FOREACH_VAL(e, &offs, sid)
                        J1S(tst, queset, sid);
                INVA_FOREACH_VAL_END
        }

        /* fill the queset */
        for (i = 0; i < cues->len; i++){
                
                u32 cue = cues->lst[i];
                
                if (cue > XID_TOKEN_FREQUENT_F &&
                    cue <= XID_TOKEN_FREQUENT_L)
                        continue;
                
                if (qexp_loaded){
                        const glist *exp = get_expansion(cue);
                        if (exp)
                                for (j = 0; j < exp->len; j++)
                                        add_to_queset(exp->lst[j]);
                }
                add_to_queset(cue);
        }
        return queset;
}

