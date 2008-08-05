/*
 *   indexing/query_proc.c
 *   Query processor
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
#include <time.h>

#include <dub.h>
#include <ixemes.h>
#include <gglist.h>
#include <qexpansion.h>
//#include <pcode.h>
#include <rrice.h>

#include <Judy.h>

#include <dexvm.h>
#include "query_proc.h"

#define IX_CLEAN 1
#define IX_UNION 0

#define TIMER_DEF clock_t __start;
#define TIMER_START __start = clock();
#define TIMER_END(msg) dub_msg(msg,\
        ((double) (clock() - __start)) / (CLOCKS_PER_SEC / 1000.0));


typedef struct{
        Pvoid_t set;
        uint    not;
} rset;

struct phrase_key{
        Pvoid_t key;
        char    *str;
};

static inline void op_ixeme(u32 xid, rset *dst, uint clean, int did_mode);
static inline void op_exp_ixeme(u32 xid, rset *dst, int did_mode);
static inline void op_range(const u32 *arg, uint len, rset *dst, int did_mode);
static inline void op_phrase(const u32 *phrase, 
        uint len, rset *dst, int did_mode);
static inline void op_and(rset *sets, int did_mode);
//static struct phrase_key *new_phrase_key(const u32 *phrase, uint len);
//static void free_phrase_key(struct phrase_key *key);
//static uint contains_phrase(u32 did, const struct phrase_key *key);
static Pvoid_t isect(const rset *big, const rset *smal);
static Pvoid_t isect_not(const rset *big, const rset *smal);

//static Pvoid_t find_bounding_segments(u32 first_xid, 
//        u32 last_xid, Pvoid_t resultset);
static Pvoid_t find_bounding_segments(const u32 *phrase, int len);
//static int phrase_in_segment(u32 sid, const u32 *phrase, int len, u32 *aux);

static int phrase_in_segment(u32 sid, const u32 *phrase, 
        u32 *last_xids, int len);

glist *do_query(const glist *keys, uint did_mode)
{
        glist *ret;
        Word_t idx = 0;
        
        rset stack[3];
        uint stack_ptr = 0;
        
        const u32 *seq_ptr = NULL;
        uint      seq_len  = 0;
        uint      sze, i, tst;

        stack->set = NULL;
        stack->not = 0;

        for (i = 0; i < keys->len; i++){
               
                dub_msg("OP %u", keys->lst[i]);
                
                switch (keys->lst[i]){

                        case XID_QOP_NOT:
                                stack[stack_ptr - 1].not = 1;
                                break;

                        case XID_QOP_AND:
                                /*
                                op_and(&stack[stack_ptr - 2]);
                                --stack_ptr;
                                */
                                break;

                        case XID_QOP_ALL:
                                /* Everything is the complement 
                                 * of nothingness */
                                op_ixeme(0, &stack[stack_ptr], 
                                        IX_CLEAN, did_mode);
                                stack[stack_ptr++].not = 1;
                                break;
                        
                        case XID_QOP_SEQ:
                                seq_len = 0;
                                seq_ptr = &keys->lst[i + 1];
                                break;

                        case XID_QOP_PHA:
                                op_phrase(seq_ptr, seq_len, 
                                        &stack[stack_ptr++], did_mode);
                                seq_len = 0;
                                seq_ptr = NULL;
                                break;

                        case XID_QOP_RGE:
                                op_range(seq_ptr, seq_len,
                                         &stack[stack_ptr++], did_mode);
                                seq_len = 0;
                                seq_ptr = NULL;
                                break;
                        
                        default: /* IXEME */
                                if (seq_ptr)
                                        ++seq_len;
                                else if (qexp_loaded)
                                        op_exp_ixeme(keys->lst[i], 
                                                &stack[stack_ptr++], did_mode);
                                else
                                        op_ixeme(keys->lst[i], 
                                                &stack[stack_ptr++], IX_CLEAN,
                                                        did_mode);
                }
                
                if (stack_ptr == 2){
                        op_and(&stack[stack_ptr - 2], did_mode);
                        --stack_ptr;
                }
        }

        dub_dbg("DOne");
        
        if (did_mode){
                Pvoid_t sidset = NULL;
                idx = 0;
                J1F(tst, stack->set, idx);
                while (tst){
                        u32 sid;
                        for (sid = DID2SID(idx); sid < DEX_NOF_SEG &&
                                SID2DID(sid) == idx; sid++){
                                
                                J1S(tst, sidset, sid);
                        }
                        J1N(tst, stack->set, idx);
                }
                J1FA(tst, stack->set);
                stack->set = sidset;
        }

        J1C(sze, stack->set, 0, -1);
        if (stack->not)
                sze = DEX_NOF_SEG - sze;

        ret = xmalloc(sze * 4 + sizeof(glist));
        ret->len = sze;
        i = idx = 0;

        if (stack->not){
                J1FE(tst, stack->set, idx);
                while (tst){
                        ret->lst[i++] = idx;  
                        J1NE(tst, stack->set, idx);
                        if (idx >= DEX_NOF_SEG)
                                break;
                }
        }else{
                J1F(tst, stack->set, idx);
                while (tst){
                        ret->lst[i++] = idx;
                        J1N(tst, stack->set, idx);
                }
        }

        dub_msg("IO %u LEN %u", i, ret->len);
                
        J1FA(i, stack->set);
        return ret;
}

static void op_ixeme(u32 xid, rset *dst, uint clean, int did_mode)
{
        const inva_e *e = NULL;
        u32 sid;
        u32 offs = 0;
        int tst;                

        if (clean)
                memset(dst, 0, sizeof(rset));
        
        if (inva_find(xid, &e) == -1)
                return;

        if (did_mode){
                INVA_FOREACH_VAL(e, &offs, sid)
                        J1S(tst, dst->set, SID2DID(sid));
                INVA_FOREACH_VAL_END
        }else{
                INVA_FOREACH_VAL(e, &offs, sid)
                        J1S(tst, dst->set, sid);
                INVA_FOREACH_VAL_END
        }
}

/*
static inline void op_ixeme_did(u32 xid, rset *dst)
{
        const inva_e *e = NULL;
        u32 sid;
        u32 offs = 0;
        int tst;                

        memset(dst, 0, sizeof(rset));
        
        if (inva_find(xid, &e) == -1)
                return;

        INVA_FOREACH_VAL(e, &offs, sid)
                J1S(tst, dst->set, SID2DID(sid));
        INVA_FOREACH_VAL_END
}
*/

static void op_exp_ixeme(u32 xid, rset *dst, int did_mode)
{
        const glist *exp = NULL;
        uint i;

        op_ixeme(xid, dst, IX_CLEAN, did_mode);

        exp = get_expansion(xid);
        if (!exp)
                /* no expansion for this ixeme */
                return;

        for (i = 0; i < exp->len; i++)
                op_ixeme(exp->lst[i], dst, IX_UNION, did_mode);
}

static inline void op_and(rset *sets, int did_mode)
{
        Pvoid_t isct = NULL;
        
        rset *big  = NULL;
        rset *smal = NULL;
        int sze1, sze2;
        
        J1C(sze1, sets[0].set, 0, -1);
        J1C(sze2, sets[1].set, 0, -1);

        if (sets[0].not)
                sze1 = (did_mode ? DEX_NOF_DOX: DEX_NOF_SEG) - sze1;
        if (sets[1].not)
                sze2 = (did_mode ? DEX_NOF_DOX: DEX_NOF_SEG) - sze2;
        
        if (sze1 > sze2){
                big  = &sets[0];
                smal = &sets[1];
        }else{
                big  = &sets[1];
                smal = &sets[0];
        }

        if (smal->not)
                isct = isect_not(big, smal);
        else
                isct = isect(big, smal);

        J1FA(sze1, big->set);
        J1FA(sze1, smal->set);

        sets->set = isct;
        sets->not = 0;
}

static void op_range(const u32 *arg, uint len, rset *dst, int did_mode)
{
        u32 xid;
        
        memset(dst, 0, sizeof(rset));
        
        if (len != 2)
                return;

        for (xid = arg[0]; xid <= arg[1]; xid++)
                op_ixeme(xid, dst, IX_UNION, did_mode);
}

static void op_phrase(const u32 *phrase, uint len, rset *dst, int did_mode)
{
        static u32 *phrase_aux;
        static uint phrase_aux_len;

        if (phrase_aux_len < len){
                phrase_aux = realloc(phrase_aux, len * 4);
                phrase_aux_len = len;
        }

        int i, tst;
        
        /* trivial cases */
        if (len < 2){
                if (!len)
                        memset(dst, 0, sizeof(rset));
                else
                        op_ixeme(phrase[0], dst, IX_CLEAN, did_mode);
                        return;
        }

        Pvoid_t hits = find_bounding_segments(phrase, len);
        memset(phrase_aux, 0, len * 4);

        u32 prev_did = 0;
        Pvoid_t resultset = NULL;
        Word_t sid = 0;
        Word_t *ptr;
        JLF(ptr, hits, sid);
        while (ptr){
                u32 did = SID2DID(sid);
                if (did_mode){
                        J1T(tst, resultset, did);
                        if (tst)
                                goto next_segment;
                }
                if (did != prev_did){
                        memset(phrase_aux, 0, len * 4);
                        prev_did = did;
                }
                int r = phrase_in_segment(sid, phrase, phrase_aux, len);
                if (r){
                        u32 id = did_mode ? did: sid;
                        J1S(tst, resultset, id);
                }
next_segment:
                JLN(ptr, hits, sid);
        }

        J1C(i, resultset, 0, -1);
        dub_msg("hits %u", i);

        dst->set = resultset;
        dst->not = 0;
        JLFA(tst, hits);
}

static Pvoid_t find_bounding_segments(const u32 *phrase, int len)
{
        Pvoid_t hits = NULL;
        Word_t *ptr;

        u32 tst, i, max_idx, max_xid = max_idx = 0;
        for (i = 0; i < len; i++)
                if (phrase[i] > max_xid){
                        max_xid = phrase[i];
                        max_idx = i;
                }

        const inva_e *e = NULL;
        if (inva_find(max_xid, &e) == -1)
                return NULL;
        
        u32 min_sid, max_sid, sid, offs = max_sid = 0;
        INVA_FOREACH_VAL(e, &offs, sid)
                if (sid){
                        JLI(ptr, hits, sid - 1);
                        ++*ptr;
                }
                JLI(ptr, hits, sid);
                ++*ptr;
                if (sid + 1 < DEX_NOF_SEG){
                        JLI(ptr, hits, sid + 1);
                        ++*ptr;
                }
                max_sid = sid;
        INVA_FOREACH_VAL_END

        min_sid = 0;
        JLF(ptr, hits, min_sid);
        u32 target_len = len;
        
        for (i = 0; i < len; i++){
                if (i == max_idx)
                        continue;
               
                /* rationale: We can assume that frequent ixemes occur
                 * in almost every document. Thus going through the inverted
                 * list of a frequent ixeme wouldn't constrain the hit set
                 * much at all, although it is expensive. Hence we skip all
                 * frequent ixemes. */
                if (phrase[i] < XID_TOKEN_FREQUENT_L){
                        --target_len;
                        continue;
                }
                
                if (inva_find(phrase[i], &e) == -1){
                        JLFA(tst, hits);
                        return NULL;
                }
                
                offs = 0;
                INVA_FOREACH_VAL(e, &offs, sid)
                        if (sid < min_sid)
                                continue;
                        if (sid > max_sid)
                                break;
                        JLG(ptr, hits, sid);
                        if (ptr){
                                ++*ptr;
                                JLG(ptr, hits, sid - 1);
                                if (ptr) ++*ptr;
                                JLG(ptr, hits, sid + 1);
                                if (ptr) ++*ptr;
                        }
                INVA_FOREACH_VAL_END
        }

        sid = 0;
        JLF(ptr, hits, sid);
        while (ptr){
                if (*ptr < target_len){
                        JLD(i, hits, sid);
                }       
                JLN(ptr, hits, sid);
        }
        return hits;
}

#if 0
static Pvoid_t find_bounding_segments(const u32 first_xid, 
        u32 last_xid, Pvoid_t resultset)
{
        rset sets[2];
        rset *big  = NULL;
        rset *smal = NULL;
        int tst, sze1, sze2, mod;
        Word_t sid;

        op_ixeme(first_xid, &sets[0], IX_CLEAN, 0);
        op_ixeme(last_xid, &sets[1], IX_CLEAN, 0);

        J1C(sze1, sets[0].set, 0, -1);
        J1C(sze2, sets[1].set, 0, -1);

        if (sze1 > sze2){
                big  = sets[0].set;
                smal = sets[1].set;
                mod = -1;
                sid = 1;
        }else{
                big  = sets[1].set;
                smal = sets[0].set;
                mod = 1;
                sid = 0;
        }
        
        J1F(tst, smal, sid);
        while (tst){
                if (SID2DID(sid) == SID2DID(sid + mod)){
                        J1T(tst, big, sid + mod);
                        if (tst){
                                J1S(tst, resultset, sid + mod);
                                J1S(tst, resultset, sid);
                        }
                }
                J1N(tst, smal, sid);
        }

        J1FA(tst, sets[0].set);
        J1FA(tst, sets[1].set);

        return resultset;
}
#endif

static int phrase_in_segment(u32 sid, const u32 *phrase, 
        u32 *last_xids, int len)
{
        u32 i, j, k, found, offs = 0;
        
        const void *xidlst = fetch_item(POS, sid);
        const void *poslst = &xidlst[dex_section[POS].toc[sid].val];
        
        static u32 **phrase_pos;
        static u32 *poslst_len;
        static u32 buf_len;

        if (buf_len < len){
                phrase_pos = realloc(phrase_pos, len * sizeof(u32*));
                poslst_len = realloc(poslst_len, len * 4);
                buf_len = len;
        }
        memset(phrase_pos, 0, len * sizeof(u32*));
        memset(poslst_len, 0, len * 4);

        u32 check_pos(u32 is_pos, u32 phrase_idx){
                if (phrase_idx == len)
                        return phrase_idx;

                for (k = 0; k < poslst_len[phrase_idx] &&
                        phrase_pos[phrase_idx][k] <= is_pos; k++)
                        
                        if (phrase_pos[phrase_idx][k] == is_pos)
                                return check_pos(is_pos + 1, phrase_idx + 1);
                return phrase_idx;
        }

        u32 tlen = elias_gamma_read(xidlst, &offs) - 1;
        //dub_msg("SID %u TLEN %u", sid, tlen);
        if (!tlen)
                return 0;

        u32 max_xid = 0;
        for (i = 0; i < len; i++)
                if (phrase[i] > max_xid)
                        max_xid = phrase[i];

        u32 rice_f1 = rice_read(xidlst, &offs, 2);
        u32 rice_f2 = rice_read(xidlst, &offs, 2);

/* Step 1:
 * Decode position lists for all the phrase ixemes. E.g:
 * united -> [1, 4, 6, 10]
 * states -> [9, 7]
*/
        u32 xid_seen, xid = 0;
        u32 poffs = xid_seen = 0;
        while (tlen--){
                xid += rice_read(xidlst, &offs, rice_f1);
                if (xid > max_xid)
                        break;
                poffs += rice_read(xidlst, &offs, rice_f2);
                for (i = 0; i < len; i++)
                        if (xid == phrase[i]){
                                u32 tmp = 0;
                                u32 tmp_offs = poffs;
                                poslst_len[i] =
                                        rice_decode(&phrase_pos[i], &tmp, 
                                                poslst, &tmp_offs);
                                if (++xid_seen == len)
                                        goto check_phrase;
                        }
        }

check_phrase:
        found = 0;
        
/* Step 2:
 * Check if the beginning of this segment completes the phrase that started
 * in the end of the previous segment (last_xids). */

        //dub_msg("last_xids[0] %u", last_xids[0]);
        for (i = 0; i < len && last_xids[i] == phrase[0]; i++){
                //dub_msg("last xid[%u] = %u", i, last_xids[i]);
                for (j = 1; j < len && last_xids[i + j] == phrase[j]; j++);
                if (check_pos(1, j) == len){
                        found = 1;
                        break;
                }
        }

/* Step 3:
 * Check if this segment includes the phrase in full. */

        if (xid_seen == len && !found)
                for (i = 0; i < poslst_len[0]; i++)
                        if (check_pos(phrase_pos[0][i] + 1, 1) == len){
                                found = 1;
                                break;
                        }

/* Step 4:
 * If the end of this segment contains a prefix of the phrase,
 * save it to last_xids. */

        memset(last_xids, 0, len * 4);
        for (i = 0; i < poslst_len[0]; i++)
                if (phrase_pos[0][i] >= dex_header.segment_size - (len - 1)){
                        u32 fits = dex_header.segment_size - phrase_pos[0][i];
                        if (check_pos(phrase_pos[0][i] + 1, 1) > fits){
                                memcpy(last_xids, phrase, (fits + 1) * 4);
                                break;
                        }
                }

        for (i = 0; i < len; i++)
                free(phrase_pos[i]);

        return found;
}

#if 0
static void xids_to_idxs(const Pvoid_t mapping, const u32 *xids, u32 *idxs, int len)
{
        int tst, i;
        for (i = 0; i < len; i++){
                J1T(tst, mapping, xids[i]);
                if (tst){
                        J1C(tst, mapping, 0, xids[i]);
                        idxs[i] = tst;
                }else
                        idxs[i] = 0;
        }
}

static Pvoid_t xid_mapping(u32 sid, const u32 *phrase, int len)
{
        int tst, i, layer = NUM_FW_LAYERS;
        int xid_seen = 0;

        Pvoid_t mapping = NULL;
        while (layer--){
                const void *p = fw_find(layer, sid);
                if (!p)
                        continue; 

                u32 xid, offs = 0;

                DEX_FOREACH_VAL(p, &offs, xid)
                        J1S(tst, mapping, xid);
                        /*
                        for (i = 0; i < len; i++)
                                if (phrase[i] == xid && ++xid_seen == len)
                                        return mapping;
                        */
                DEX_FOREACH_VAL_END
        }
        return mapping;
}

static void make_phrase_keys(const Pvoid_t segs, u32 *phrase, int len)
{
        int j, i, num_segs; 
        J1C(num_segs, segs, 0, -1);
        u32 *keys = xmalloc(num_segs * len * 4);
        memset(keys, 0, num_segs * len * 4);
        
        u32 max_xid = 0;
        for (i = 0; i < len; i++)
                if (phrase[i] > max_xid)
                        max_xid = phrase[i];

        int layer = NUM_FW_LAYERS;
        while (layer--){
                dub_msg("LAyer %u (%u)", layer,
                        dex_header.fw_layers[layer].min_xid);
                if (layer < 9 || dex_header.fw_layers[layer].min_xid > max_xid){
                        dub_msg("Skipping layer %u (min_xid: %u)",
                                layer, dex_header.fw_layers[layer].min_xid);
                        continue;
                }

                Word_t sid = i = 0;
                int tst;
                J1F(tst, segs, sid);
                while (tst){
                        const void *p = fw_find(layer, sid);
                        if (!p)
                                goto next_seg;

                        u32 xid, offs = 0;

                        DEX_FOREACH_VAL(p, &offs, xid)
                                if (xid > max_xid)
                                        break;
                                for (j = 0; j < len; j++)
                                        if (xid <= phrase[j])
                                                ++keys[i * len + j];
                        DEX_FOREACH_VAL_END

                        ++i;
next_seg:
                        J1N(tst, segs, sid);
                }
        }

        return keys;
}

static int phrase_in_segment3(u32 sid, const u32 *phrase, int len, u32 *aux)
{
        static u32 *phrase_idx;
        static uint phrase_idx_len;
        static u32 *aux2;

        int i;

        if (phrase_idx_len < len){
                phrase_idx = realloc(phrase_idx, len * 4);
                aux2 = realloc(aux2, len * 4);
                phrase_idx_len = len;
        }

        //Pvoid_t mapping = xid_mapping(sid, phrase, len);
        //xids_to_idxs(mapping, phrase, phrase_idx, len);



        //inline void rice_write(char *dest, u32 *offs, u32 val, uint f);

        //J1FA(i, mapping);
        return 0;
}

static int phrase_in_segment2(u32 sid, const u32 *phrase, int len, u32 *aux)
{
        static u32 *phrase_idx;
        static uint phrase_idx_len;
        static u32 *aux2;

        if (phrase_idx_len < len){
                phrase_idx = realloc(phrase_idx, len * 4);
                aux2 = realloc(aux2, len * 4);
                phrase_idx_len = len;
        }

        int found, i, j = 0;
        u32 idx, offs = found = 0;
        Pvoid_t mapping = NULL;//xid_mapping(sid, phrase, len);
        /* 
        xids_to_idxs(mapping, phrase, phrase_idx, len);
        xids_to_idxs(mapping, aux, aux, len);
        */
        const void *p = fetch_item(POS, sid);
        /* Naive string matching algorithm: Find the substring docphrase in
         * the position list. Takes O(len*n). */
        //const u32 *h = (const u32*)p;
        //for (i = 0; i < 100; i++)
        //        j += h[i];
        POS_FOREACH_VAL(p, &offs, idx)
                aux[j++] = idx;
                if (j == len)
                        j = 0;
                /*
                if (!found){
                        for (i = 0; i < len &&
                                aux[(i + j) % len] == phrase_idx[i]; i++);
                        if (i == len)
                                found = 1;
                }
                */
        POS_FOREACH_VAL_END
        //dub_msg("OFFS %u", offs / 8);
        memset(aux2, 0, len * 4);
        for (i = 0; i < len; i++){
                int tst, k;
                Word_t xid;
                for (k = 0; k < len; k++)
                        if (phrase_idx[k] == aux[(i + j) % len]){
                                aux2[i] = phrase[k];
                                break;
                        }
                //J1BC(tst, mapping, (aux[(i + j) % len] + 1), xid);
                //phrase_idx[i] = tst ? xid: 0;
        }
        memcpy(aux, phrase_idx, len * 4);
        J1FA(i, mapping);
        return found;
}

static void pos_xid_mapping(u32 did, const u32 *phrase, u32 *ret, int len)
{
        Pvoid_t mapping = NULL;
        u32 tst, layer, sid, first_sid = DOC_FIRST_SID(did);

        for (layer = 0; layer < NUM_FW_LAYERS; layer++){
                for (sid = first_sid; sid < DEX_NOF_SEG &&
                                SID2DID(sid) == did; sid++){

                        const void *p = fw_find(layer, sid);
                        if (!p)
                                continue;
                        u32 xid, offs = 0;
                        DEX_FOREACH_VAL(p, &offs, xid)
                                J1S(tst, mapping, xid);
                        DEX_FOREACH_VAL_END       
                }
        }
        
        Word_t max;
        J1C(max, mapping, 0, -1);

        int i;
        for (i = 0; i < len; i++){
                Word_t c;
                J1C(c, mapping, 0, phrase[i]);
                if (c == max)
                        dub_die("Error: XID %u missing from DID %u",
                                phrase[i], did);
                ret[i] = c;
        }
        J1FA(tst, mapping);
}



static struct phrase_key *new_phrase_key(const u32 *phrase, uint len)
{
        struct phrase_key *key = xmalloc(sizeof(struct phrase_key));
        Word_t  *ptr;
        uint i, j;
       
        key->key = NULL;
        
        /* let's stay in 'signed char' scale. Values 0 and 1 have special
         * meanings, thus 126 characters are left to denote ixemes. */
        if (len > 126)
                dub_die("Maximum phrase length is 126 ixemes");
       
        /* form the search key */
        key->str = xmalloc(len + 1);
        for (j = 2, i = 0; i < len; i++){
                JLI(ptr, key->key, phrase[i]);
                if (!*ptr)
                        *ptr = j++;
                key->str[i] = *ptr;
        }
        key->str[i] = 0;
        
        return key;
}

static void free_phrase_key(struct phrase_key *key)
{
        uint tmp;
        
        JLFA(tmp, key->key);
        free(key->str);
        free(key);
}

/* this function ensures that phrase appears in that exact order
 * in document did */
static uint contains_phrase(u32 did, const struct phrase_key *key)
{
        static u32  *doc_ix;
        static uint doc_ix_len;
        
        u32     offs = 0;
        Pvoid_t jstr = NULL;
        Word_t  *ptr;
        Word_t  idx = 0;
        uint    prev = 0;
        char    *str;
        uint    ixlen, i, j;

        const void *p = fetch_item(POS, did);
      
        /* list of ixemes in this document */
        ixlen = rice_decode(&doc_ix, &doc_ix_len, p, &offs);

        JLC(j, key->key, 0, -1);
        
        /* decode positions for the phrase ixemes */
        for (i = 0; i < ixlen && j; i++){

                u32 val;
                Word_t *keyval;
                
                JLG(keyval, key->key, doc_ix[i]);
                if (keyval){
                        /* this ixeme in the phrase: decode positions */
                        
                        DEX_FOREACH_VAL(p, &offs, val)
                                JLI(ptr, jstr, val)
                                *ptr = *keyval;
                        DEX_FOREACH_VAL_END
                        --j;
                }else{
                        /* this ixeme not in the phrase: skip it */
                        DEX_FOREACH_VAL(p, &offs, val)
                        DEX_FOREACH_VAL_END
                }
        }

        /* form a pseudo-string representing ixeme positions in the doc */
        JLC(i, jstr, 0, -1);

        str = xmalloc(i * 2 + 1);
        i = 0;
        
        JLF(ptr, jstr, idx);
        prev = idx - 1;
        while (ptr != NULL){
               
                /* add a delimiter if the current ixeme isn't consecutive
                 * to the previous one */
                if (prev != idx - 1)
                        str[i++] = 1;
                
                str[i++] = *ptr;
                prev = idx;
                JLN(ptr, jstr, idx);
        }
        str[i] = 0;

        /* efficiency of this function boils down to the efficiency of the 
         * following strstr() implementation */
        j = (strstr(str, key->str) != NULL);

        free(str);
        JLFA(i, jstr);

        dub_msg("Contains phrase Result %u", j);
        
        return j;
}
#endif 

static Pvoid_t isect(const rset *big, const rset *smal)
{
        Pvoid_t res = NULL;
        Word_t  idx = 0;
        int tst;

        J1F(tst, smal->set, idx);

        if (big->not){
                while (tst){
                        J1T(tst, big->set, idx);
                        if (!tst){
                                J1S(tst, res, idx);
                        }
                        J1N(tst, smal->set, idx);
                }
        }else{
                while (tst){
                        J1T(tst, big->set, idx);
                        if (tst){
                                J1S(tst, res, idx);
                        }
                        J1N(tst, smal->set, idx);
                }
        }

        return res;
}

static Pvoid_t isect_not(const rset *big, const rset *smal)
{
        Pvoid_t res = NULL;
        Word_t  idx = 0;
        int tst;

        J1FE(tst, smal->set, idx);

        if (big->not){
                while (tst){
                        J1T(tst, big->set, idx);
                        if (!tst){
                                J1S(tst, res, idx);
                        }
                        
                        J1NE(tst, smal->set, idx);
                        if (idx >= DEX_NOF_DOX)
                                break;
                }
        }else{
                while (tst){
                        J1T(tst, big->set, idx);
                        if (tst){
                                J1S(tst, res, idx);
                        }
                        
                        J1NE(tst, smal->set, idx);
                        if (idx >= DEX_NOF_DOX)
                                break;
                }
        }
        
        return res;

}

