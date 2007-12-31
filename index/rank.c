/*
 *   indexing/rank.c
 *   Rank a set of documents
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


#include <rrice.h>
#include <dexvm.h>
#include <gglist.h>
#include <qexpansion.h>
#include <pparam.h>
#include <bbitmap.h>

#include <Judy.h>
#include <string.h>
#include <time.h>

#include "scorelib.h"
#include "query_proc.h"
#include <fibheap.h>

#define TIMER_DEF clock_t __start;
#define TIMER_START __start = clock();
#define TIMER_END(msg) dub_msg(msg,\
        ((double) (clock() - __start)) / (CLOCKS_PER_SEC / 1000.0));

#define TOPSET_SIZE 10
#define HORIZON_SIZE 90

#define FIRST_SWEEPED_LAYER (HIGH_LAYER_LIMIT - 3)
#define SWEEP_CHECK_LIMIT 500
#define SWEEP_HIGH_PERCENTAGE 0.5

#define LOOKUP_TABLE_SIZE 2000

#define MULT ((float)1E6)

typedef struct{
        u32 key_offs;
        u32 max_key;
        float scores[LOOKUP_TABLE_SIZE];
} lookup_table_t;

struct scoreset{
        Pvoid_t segs;
        fibheap_t fib;
        float min;
        int nof_elems;
        int max_elems;
};

static struct scored_doc *document_scores(Pvoid_t seg_scores);

static int score_cmp(const void *i1, const void *i2)
{
        float score1 = ((const struct scored_doc*)i1)->score;
        float score2 = ((const struct scored_doc*)i2)->score;
        
        if (score1 < score2)
                return 1;
        else if (score1 > score2)
                return -1;
        return 0;
}

glist *find_hits(const glist *keys, uint did_mode)
{
        return do_query(keys, did_mode);
}

static float seg_score(u32 sid, const Pvoid_t *layer_scores, 
                        int first_layer, int last_layer)
{
        float score = 0;
        int layer;

        for (layer = first_layer; layer < last_layer; layer++){
                const void *p = fw_find(layer, sid);
                if (!p)
                        continue;
                
                const Pvoid_t ixscores = layer_scores[layer];
                u32 offs = 0;
                u32 xid;
                Word_t *sco;

                DEX_FOREACH_VAL(p, &offs, xid)
                        JLG(sco, ixscores, xid);
                        if (sco)
                                score += *(float*)sco;
                DEX_FOREACH_VAL_END
        }
        return score;
}


static const lookup_table_t *make_lookup_table(const Pvoid_t scores)
{
        static lookup_table_t table;
        Word_t *ptr;
        Word_t key = 0;
        JLF(ptr, scores, key);
        table.key_offs = key;
        while (ptr){
                if (key - table.key_offs >= LOOKUP_TABLE_SIZE)
                        break;
                table.scores[key - table.key_offs] = *(float*)ptr;
                table.max_key = key;
                JLN(ptr, scores, key);
        }
        return &table;
}

static int update_set(struct scoreset *s, Word_t sid, float score)
{
        Word_t *ptr;
        JLI(ptr, s->segs, sid);
        if (*ptr)
                fibheap_delete_node(s->fib, (fibnode_t)*ptr);
        else
                ++s->nof_elems;

        *ptr = (Word_t)fibheap_insert(s->fib, score * MULT, (void*)sid);

        if (s->nof_elems > s->max_elems){
                int tst;
                sid = (u32)fibheap_extract_min(s->fib);
                JLD(tst, s->segs, sid);
                --s->nof_elems;
                s->min = fibheap_min_key(s->fib) / MULT;
        }
        
        return 1;
}

static Pvoid_t sweep_high_layers(const glist *hits, 
        const Pvoid_t *layer_scores)
{
        Pvoid_t seg_scores = NULL;
        int layer, i;

        for (layer = FIRST_SWEEPED_LAYER; layer < HIGH_LAYER_LIMIT; layer++){
                
                const Pvoid_t ixscores = layer_scores[layer];
                const lookup_table_t *table = make_lookup_table(ixscores);
                
                for (i = 0; i < hits->len; i++){
                        u32 sid = hits->lst[i];
                        const void *p = fw_find(layer, sid);
                        if (!p)
                                continue;
                        float score = 0;
                        u32 offs = 0;
                        u32 xid;
                        DEX_FOREACH_VAL(p, &offs, xid)
                                if (xid < table->key_offs)
                                        continue;
                                if (xid <= table->max_key)
                                        score += table->scores[
                                                xid - table->key_offs];
                                else{
                                        Word_t *sco;
                                        JLG(sco, ixscores, xid);
                                        if (sco)
                                                score += *(float*)sco;
                                }
                        DEX_FOREACH_VAL_END
                        Word_t *ptr;
                        JLI(ptr, seg_scores, sid);
                        float sco = (*(float*)ptr) + score;
                        memcpy(ptr, &sco, sizeof(float));
                }
        }
        return seg_scores;
}

static void update_scoresets(const Pvoid_t seg_scores, 
                             const Pvoid_t *layer_scores,
                             struct scoreset *topset, 
                             struct scoreset *rankset,
                             bmap_t *hitmap)
{
        Word_t *ptr;
        Word_t sid = 0;
        JLF(ptr, seg_scores, sid);
        while (ptr){
                float dprior = DOCPRIOR(sid);
                float sco = *(float*)ptr * dprior;

                if (rankset->min < sco)
                        update_set(rankset, sid, sco);

                if (topset->min < sco){
                        bmap_unset(hitmap, sid);
                        sco += dprior * seg_score(sid, 
                                layer_scores, 0, FIRST_SWEEPED_LAYER);
                        update_set(topset, sid, sco);
                }
                memcpy(ptr, &sco, sizeof(float));
                JLN(ptr, seg_scores, sid);
        }
}

static inline int in_high_layer(u32 xid, const Pvoid_t *layer_scores)
{
        int layer;
        for (layer = FIRST_SWEEPED_LAYER; layer < HIGH_LAYER_LIMIT; layer++){
                Word_t *ptr;
                JLG(ptr, layer_scores[layer], xid);
                if (ptr)
                        return 1;
        }
        return 0;
}

Pvoid_t combine_layers(const Pvoid_t *layer_scores)
{
        Pvoid_t scores = NULL;
        int layer;
        for (layer = 0; layer < HIGH_LAYER_LIMIT; layer++){
                const Pvoid_t ixscores = layer_scores[layer];
                Word_t xid = 0;
                Word_t *ptr, *ptr2;
                JLF(ptr, ixscores, xid);
                while (ptr){
                        JLI(ptr2, scores, xid);
                        *ptr2 = *ptr;
                        JLN(ptr, ixscores, xid);
                }
        }
        return scores;
}

static bmap_t *make_hitmap(const glist *hits)
{
        int i;
        u32 max = 0;
        for (i = 0; i < hits->len; i++)
                if (hits->lst[i] > max)
                        max = hits->lst[i];
        
        bmap_t *hitmap = bmap_init(max);
        for (i = 0; i < hits->len; i++){
                bmap_set(hitmap, hits->lst[i]);
        }
        
        return hitmap;
}


#if 0
static Pvoid_t find_rankset(const bmap_t *hits_o, int layer, 
        Pvoid_t *seg_scores_p, Pvoid_t ixscores, 
        const struct scored_ix *ixscores_sorted,
        const Pvoid_t *layer_scores,
        struct scoreset *topset, struct scoreset *rankset)
{       
        TIMER_DEF
        Word_t *ptr;
        u32 i, j, offs, fwiter, swept = 0;
        Word_t sid, num_scores;
        float win_weight = 0;

        Pvoid_t seg_scores = *seg_scores_p;

        /*
        bmap_t *hits = bmap_clone(hits_o);
      
        for (i = 0; i < HORIZON_SIZE && i < num_scores; i++)
                win_weight += ixscores_sorted[i].score;
         
        const u32 highest_sweeped_xid = 
                dex_header.fw_layers[FIRST_SWEEPED_LAYER].min_xid;
        */

        TIMER_START
        fwiter = 0;
        JLF(ptr, rankset->segs, sid);
        while (ptr){
                if (bmap_test(hits, sid)){
                        float sco = ((fibnode_t)*ptr)->key / (float)MULT;
                        float dprior = DOCPRIOR(sid);
                        //sco += dprior * seg_score(layer, &sid, 
                        //        ixscores, &fwiter);
                        ((fibnode_t)*ptr)->key = sco * MULT;
                        //JLF(ptr, rankset->segs, sid);
                        JLN(ptr, rankset->segs, sid);
                }else{
                        JLN(ptr, rankset->segs, sid);
                }
        }
        TIMER_END("Rest took %gms");

        *seg_scores_p = seg_scores;
        //JLFA(i, ixscores);
        free(hits);
        return rankset->segs;
}
#endif



/* Known problem (feature): Now ranking is totally segment-based. Since
 * multiple segments in the rankset may belong to the same document, the
 * returned ranked set of documents may be smaller than the rankset. This
 * feature becomes apparent especially of the documents are lengthy.
 * 
 * Possible fix: When a segment is promoted to the topset, compute the
 * exact scores for all the segments belonging to the corresponding
 * document. Pick the best segment score, insert this to the topset,
 * and remove all the segments of the document from the hitset. The
 * downsides are that 1) we may perform costly full scoring for documents
 * which do not end up in the rankset finally (unlikely) 2) computing
 * the exact scores for a document is expensive. However, we may save 
 * the exact scores and re-use them later when scoring the rankset.
 *
 */
struct scored_doc *rank_dyn(const glist *hits, const Pvoid_t *layer_scores)
{
        struct scoreset topset = {.segs = NULL,
                                  .fib = fibheap_new(),
                                  .min = 0,
                                  .nof_elems = 0,
                                  .max_elems = TOPSET_SIZE + 1};

        struct scoreset rankset = {.segs = NULL,
                                   .fib = fibheap_new(),
                                   .min = 0,
                                   .nof_elems = 0,
                                   .max_elems = RANKSET_SIZE};

        Pvoid_t seg_scores = NULL;
        Word_t num_scores;
        int i, j, swept = 0;
        TIMER_DEF
       
        bmap_t *hitmap = make_hitmap(hits);

        TIMER_START
        Pvoid_t ixscores = combine_layers(layer_scores);
        JLC(num_scores, ixscores, 0, -1);
        struct scored_ix *ixscores_sorted = sort_scores(ixscores);
        JLFA(j, ixscores);
        TIMER_END("Ixscoring took %gms");

        for (j = 0, i = 0; i < SWEEP_CHECK_LIMIT; i++){
                if (in_high_layer(ixscores_sorted[i].xid, layer_scores))
                        j++;
        }
        
        dub_msg("Found %u high ixemes within the top %u ixemes. "
                "Sweep threshold is %u ixemes.", j, SWEEP_CHECK_LIMIT,
                (int)(SWEEP_HIGH_PERCENTAGE * SWEEP_CHECK_LIMIT));

        if (j > SWEEP_HIGH_PERCENTAGE * SWEEP_CHECK_LIMIT){
                dub_msg("Sweeping high layers");
                
                TIMER_START
                seg_scores = sweep_high_layers(hits, layer_scores);
                TIMER_END("Sweeping took %gms");

                TIMER_START
                update_scoresets(seg_scores, layer_scores, 
                        &topset, &rankset, hitmap);
                TIMER_END("Scoreset initialization took %gms");
                swept = 1;
        }

        TIMER_START
        for (i = 0; i < num_scores; i++){
                if (swept && in_high_layer(ixscores_sorted[i].xid, 
                                layer_scores))
                        continue;
                
                float win_weight = 0;
                for (j = 0; j < HORIZON_SIZE && j + i < num_scores; j++)
                        win_weight += ixscores_sorted[i + j].score;
                
                if (rankset.nof_elems == RANKSET_SIZE && 
                    rankset.min + win_weight < topset.min)
                        break;
               
                const inva_e *e;
                inva_find(ixscores_sorted[i].xid, &e);
                if (!e)
                        continue;
        
                u32 sid, offs = 0;
                INVA_FOREACH_VAL(e, &offs, sid)
                        if (!bmap_test(hitmap, sid))
                              continue;
                        
                        Word_t *ptr;
                        JLI(ptr, seg_scores, sid);

                        float dprior = DOCPRIOR(sid);
                        float sco = (*(float*)ptr) + dprior *
                                ixscores_sorted[i].score;
                        
                        if (topset.min < sco){
                                float nsco = dprior * seg_score(sid, 
                                        layer_scores, 0, HIGH_LAYER_LIMIT);
                                if (topset.min < nsco){
                                        sco = nsco;
                                        bmap_unset(hitmap, sid);
                                        update_set(&topset, sid, sco);
                                }
                        }
                        if (rankset.min < sco)
                                update_set(&rankset, sid, sco);
                                
                        memcpy(ptr, &sco, sizeof(float));

                INVA_FOREACH_VAL_END
                
                
        }
        TIMER_END("Thresholding took %gms");
        dub_msg("Scored %u / %u", i, num_scores);

        TIMER_START

        glist *rank = xmalloc(RANKSET_SIZE * 4 + sizeof(glist));
        Word_t *ptr;
        Word_t sid = i = 0;
        JLF(ptr, rankset.segs, sid);
        while (ptr){
                rank->lst[i++] = sid;
                JLN(ptr, rankset.segs, sid);
        }
        rank->len = i;
        struct scored_doc *ranked = rank_brute(rank, layer_scores);
        
        free(ixscores_sorted);
        free(rank);
        free(hitmap);
        JLFA(i, seg_scores);
        JLFA(i, topset.segs);
        fibheap_delete(topset.fib);
        JLFA(i, rankset.segs);
        fibheap_delete(rankset.fib);
        TIMER_END("Finalization took %gms");
        
        return ranked;
}

struct scored_doc *rank_brute(const glist *hits, const Pvoid_t *layer_scores)
{
        TIMER_DEF
        int i, layer;

        dub_msg("Brute ranking %u hits", hits->len);

        TIMER_START
        Pvoid_t seg_scores = sweep_high_layers(hits, layer_scores);
        TIMER_END("Sweeping high layers took %gms");
        
        TIMER_START
        for (layer = 0; layer < FIRST_SWEEPED_LAYER; layer++){
                const Pvoid_t ixscores = layer_scores[layer];
                
                for (i = 0; i < hits->len; i++){
                        u32 sid = hits->lst[i];
                        const void *p = fw_find(layer, sid);
                        if (!p)
                                continue;
                        
                        float score = 0;
                        u32 offs = 0;
                        u32 xid;
                        
                        DEX_FOREACH_VAL(p, &offs, xid)
                                Word_t *sco;
                                JLG(sco, ixscores, xid);
                                if (sco)
                                        score += *(float*)sco;
                        DEX_FOREACH_VAL_END
                        
                        Word_t *ptr;
                        JLI(ptr, seg_scores, sid);
                        float sco = (*(float*)ptr) + score;
                        memcpy(ptr, &sco, sizeof(float));
                }
        }
        TIMER_END("Low layers took %gms");

        Word_t sid = 0;
        Word_t *ptr;
        JLF(ptr, seg_scores, sid);
        while (ptr){
                float sco = DOCPRIOR(sid) * (*(float*)ptr);
                memcpy(ptr, &sco, sizeof(float));
                JLN(ptr, seg_scores, sid);
        }

        struct scored_doc *ranked = document_scores(seg_scores);
        JLFA(i, seg_scores);
        return ranked;
}

static struct scored_doc *document_scores(const Pvoid_t seg_scores)
{
        TIMER_DEF
        TIMER_START
        
        Pvoid_t doc_scores = NULL;

        Word_t num, sid;
        JLC(num, seg_scores, 0, -1);

        struct scored_doc *ranked = xmalloc((num + 1) 
                * sizeof(struct scored_doc));
        memset(ranked, 0, (num + 1) * sizeof(struct scored_doc));

        sid = 0;
        Word_t *ptr, *ptr2;
        JLF(ptr, seg_scores, sid);
        while (ptr){
                u32 did = SID2DID(sid);
                JLI(ptr2, doc_scores, did);
                float sco = (*(float*)ptr2) + (*(float*)ptr);
                memcpy(ptr2, &sco, sizeof(float));
                JLN(ptr, seg_scores, sid);
        }
        
        Word_t did = 0;
        int i = 0;
        JLF(ptr, doc_scores, did);
        while (ptr){
                ranked[i].score = *(float*)ptr;
                ranked[i++].did = did;
                JLN(ptr, doc_scores, did);
        }
        JLFA(did, doc_scores);

        qsort(ranked, i, sizeof(struct scored_doc), score_cmp);
        dub_msg("%u docs", i); 
        TIMER_END("Doc scoring took %dms")
        return ranked;
}

struct scored_doc *top_ranked(const struct scored_doc *ranked, int len, int topk)
{
        TIMER_DEF
        TIMER_START

        struct scored_doc *res = xmalloc(topk * sizeof(struct scored_doc));
        memset(res, 0, topk * sizeof(struct scored_doc));

        if (len < topk){
                memcpy(res, ranked, len * sizeof(struct scored_doc));
                return res;
        }
        
        fibheap_t fib = fibheap_new();
        int i;

        for (i = 0; i < topk; i++)
                fibheap_insert(fib, ranked[i].score * MULT, (void*)&ranked[i]);
        
        for (; i < len; i++){
                fibheap_insert(fib, ranked[i].score * MULT, (void*)&ranked[i]);
                fibheap_extract_min(fib);
        }
        while (topk--)
                res[topk] = *(struct scored_doc*)fibheap_extract_min(fib);

        fibheap_delete(fib);
        TIMER_END("Merging took %dms")
        return res;
}

