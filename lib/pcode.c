/*
 *   lib/pcode.c
 *   Encodes ascending lists of integers with a delta-coding variant
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
 *   GNU General Public License for more details.n
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* PCODE FORMAT:
 * 
 *  5 bits                 M bits        1 bit
 * ------------------------------------------------------------
 * | Entry width in bits | nof_entries | continues? | entries..
 * ------------------------------------------------------------
 *
 *  M = NOF_ENTRIES_WIDTH
 * 
 *  If the list is larger than MAX_ENTRIES_PER_SEGMENT entries, multiple
 *  headers (segments) are needed. Upped continuation flag denotes that the
 *  following segment belongs to this list.
 *
 *  If the list length is zero, header is pruned to 5 bits which are 
 *  all zero.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "dub.h"
#include "pcode.h"

/* must be a multiply of 8 */
#define DECODE_INC 1024
        
/* header cost: entry width in bits (5bits)
*              number of entries   (NOF_ENTRIES_WIDTH bits)
*              continuation flag   (1bit) */
#define HEADER_COST (6 + NOF_ENTRIES_WIDTH)

/* structs for encoding */
struct diff_e{
        uint idx;
        uint cost;        
        uint dist;
};

typedef struct{
        struct diff_e *diffs;
        uint          nof_diffs;
        uint          max_cost;
} cost_s;

void pc_print(const char *dat, u32 offs)
{
        static char buf[8192];
        
        u32  cur = 0;
        uint len = 0;
        
        PC_FOREACH_VAL(dat, &offs, cur)

                dub_dbg("ds %u", cur);
                len += snprintf(&buf[len], 8191 - len," %u", cur);

                if (len > 8191)
                        goto done;
                
        PC_FOREACH_VAL_END

done:
                
        if (len < 8191)
                dub_dbg("PCODE >> %s", buf);
        else
                dub_dbg("PCODE >> %s...", buf);
}

#if 0
uint pc_index(gen_t val, const char *dat, u32 offs)
{
        gen_t cur = (gen_t)0;
        uint  i   = 0;
        
        //dub_dbg("HE %u", val);
        
        PC_FOREACH_VAL(dat, &offs, cur)

                ++i;
          //      dub_dbg("nof %u [%u] Val %u Cur %u", __nof_ent, i, val, cur);
                if (val.gen == cur.gen)
                        return i;
                
        PC_FOREACH_VAL_END

        return 0;
}

uint pc_isect1(gen_t **stack, const char *dat1, const char *dat2,
               u32 *offs1, u32 *offs2, uint mode)
{
        uint len        = 0;
        uint stack_size = 0;
     
        uint nof_ent1 = 0;
        uint nof_ent2 = 0;
        
        uint cont1    = 1;
        uint cont2    = 1;
        
        gen_t prev1   = (gen_t)0;
        gen_t prev2   = (gen_t)0;
        
        int i1        = 1;
        int i2        = 1;
        
        static gen_t vals1[MAX_ENTRIES_PER_SEGMENT];
        static gen_t vals2[MAX_ENTRIES_PER_SEGMENT];
        
        while (1){
more_stuff:
//                dub_dbg("n1(%u) %u n2(%u) %u", nof_ent1, i1, nof_ent2, i2);
                
                if (i1 >= nof_ent1){
                        if (!cont1) goto end;
                        read_header_x(vals1, &cont1, &nof_ent1, dat1, offs1);
//                        dub_dbg("i1 update ne %u >> %u", nof_ent1, vals1[0]);
                        if (!nof_ent1) goto end;
                        prev1 = vals1[0];
                        i1 = 0;
                }
               
                if (i2 >= nof_ent2){
                        if (!cont2) goto end;
                        read_header_x(vals2, &cont2, &nof_ent2, dat2, offs2);
//                        dub_dbg("i2 update ne %u >> %u", nof_ent2, vals2[0]);
                        if (!nof_ent2) goto end;
                        prev2 = vals2[0];
                        i2 = 0;
                }
                
                while (prev1.gen < prev2.gen){
                        if (++i1 == nof_ent1)
                                goto more_stuff;
                        prev1 = vals1[i1];
                        //dub_dbg("A: i1: %u i2: %u", prev1, prev2);
                }
                
                while (prev2.gen < prev1.gen){
                        if (++i2 == nof_ent2)
                                goto more_stuff;
                        prev2 = vals2[i2];
                        //dub_dbg("B: i1: %u i2: %u", prev1, prev2);
                }
                
                if (prev1.gen == prev2.gen){
                       
                        switch (mode){

                                case 3:
                                        return 1;
        
                                case 0:
                                        ++len;
                                        break;

                                case 1:
                                        if (len == stack_size){
                                                gen_t *tmp = xmalloc((
                                                        stack_size + 
                                                        DECODE_INC) *
                                                        sizeof(gen_t));
        
                                                if (*stack){
                                                        memcpy(tmp, *stack,
                                                           stack_size * 
                                                           sizeof(gen_t));
                                                        free(*stack);
                                                }
                                                stack_size += DECODE_INC;
                                                *stack = tmp;
                                        }
                                        (*stack)[len++] = prev1;
                        }

                        if (++i1 < nof_ent1)
                                prev1 = vals1[i1];
                        if (++i2 < nof_ent2)
                                prev2 = vals2[i2];      
                }
        }
end:
//        dub_dbg("ENED");
        if (mode == 3) return 0;
        return len;
}

/* Finds the intersection of two pcodes. Returns number of overlapping
 * items. If mode == 1 the overlapping items are added to stack.
 * If mode == 2, pcodes must contain a pair of consecutive
 * items (e.g. 91 and 92 or 3 and 4) for a match to occur. */
uint pc_isect(gen_t **stack, const char *dat1, const char *dat2,
              u32 *offs1, u32 *offs2, uint mode)
{
        gen_t prev1      = (gen_t)0;
        gen_t prev2      = (gen_t)0;
        uint  cont1      = 1;
        uint  cont2      = 1;
        
        uint  nof_ent1   = 0;
        uint  nof_ent2   = 0;
        uint  ent_wdt1   = 0;
        uint  ent_wdt2   = 0;
        
        uint len         = 0;
        uint stack_size  = 0;
        
/*
        pc_print(dat1, *offs1);
        pc_print(dat2, *offs2);
        */

        /* a special case: Find consecutive matches in
         * one pcode. */
        if (mode == 2 && dat1 == dat2 && *offs1 == *offs2){
                gen_t val  = (gen_t)0;
                gen_t prev = (gen_t)0;
                
                PC_FOREACH_VAL(dat1, offs1, val)
                        
                        if (val.gen == prev.gen + 1)
                                return 1;
                        prev = val;
                        
                PC_FOREACH_VAL_END

                return 0;
        }
        
        while (cont1 || cont2){

         //       dub_dbg("KoKO");
                
                if (cont1 && !nof_ent1){
           //             dub_dbg("header 1");
                        if (!read_header(dat1, offs1, &nof_ent1,
                                        &ent_wdt1, &cont1))
                                break;
                }
                
                if (cont2 && !nof_ent2){
             //          dub_dbg("header 2");
                        if (!read_header(dat2, offs2, &nof_ent2,
                                        &ent_wdt2, &cont2))
                                break;
                }
               
                while (nof_ent1 || nof_ent2){
                        gen_t diff1 = (gen_t)0;
                        gen_t diff2 = (gen_t)0;

                        if (nof_ent1 && prev1.gen <= prev2.gen){
                                pc_read_bits32(&diff1.gen, dat1, *offs1, ent_wdt1);
                                prev1.gen += diff1.gen;
                                *offs1    += ent_wdt1;
                                --nof_ent1;
                        }

                        while (nof_ent2 && prev2.gen < prev1.gen){
                //                dub_dbg("here %u %u", prev1, prev2);
//                                
                               pc_read_bits32(&diff2.gen, dat2, *offs2, ent_wdt2);
                               prev2.gen += diff2.gen;
                               *offs2    += ent_wdt2;
                               --nof_ent2;
                        }
                  
                        /*
                        dub_dbg("Pr1 %u (%u) Pr2 %u (%u) nof1 %u nof2 %u", prev1,
                                        ent_wdt1, prev2, ent_wdt2,
                                        nof_ent1, nof_ent2);
                                        */
                        
                        if (prev1.gen == prev2.gen){

//                                dub_dbg("MATCH %u", prev1);

                                if (len == stack_size && mode == 1){
                                        gen_t *tmp = xmalloc((stack_size + 
                                                        DECODE_INC) * 
                                                        sizeof(gen_t));
                                        if (*stack){
                                                memcpy(tmp, *stack,
                                                        stack_size * 
                                                        sizeof(gen_t));
                                                free(*stack);
                                        }
                                        stack_size += DECODE_INC;
                                        *stack = tmp;
                                        
                                }else if (mode == 3)
                                        return 1;

                                if (mode == 1)
                                        (*stack)[len] = prev1;
                                ++len;
                                 
                        }else if (mode == 2 && prev1.gen + 1 == prev2.gen){
                                //dub_dbg("match %u",prev1);
                                return 1;
                        }
                
                        if (!nof_ent1 && prev2.gen >= prev1.gen)
                                break;

                        if (!nof_ent2 && prev1.gen >= prev2.gen)
                                break;
                }
                
                if (!cont1 && !nof_ent1 && prev2.gen >= prev1.gen)
                        break;

                if (!cont2 && !nof_ent2 && prev1.gen >= prev2.gen)
                        break;
        }

        if (mode == 2 || mode == 3)
                return 0;
        else
                return len;
}

uint pc_fast_forward(const char *dat, uint idx, u32 *offs)
{
        uint skipped = 0;
        
        while(idx--){
                uint cont = 1;
                while (cont){
                        uint nof_ent = 0;
                        uint ent_wdt = 0;
                        
                        if (!read_header(dat, offs, &nof_ent, &ent_wdt, &cont))
                                break;
                        
                        *offs += nof_ent * ent_wdt;
                        skipped += nof_ent;
                }
        }
        
        return skipped;
}
#endif

static inline uint bits_needed(u32 max_val)
{
        uint e_bits = pc_log2(max_val);
        if (!e_bits || max_val > ((1 << e_bits) - 1))
                ++e_bits;
        return e_bits;
}

static inline void debug_check(const u32 *pos, uint i, uint len)
{
	if (i != len - 1 && pos[i + 1] <= pos[i]){
		dub_warn("list to be pcoded is not ascending. See:");
		fprintf(stderr, "List >>");
		for (i = 0; i < len; i++)
			fprintf(stderr, " %u", pos[i]);
		dub_die("");
	}
}


/* Fill_costs() pre-calculates the encoding cost w.r.t to a specific
 * position on the list to be encoded. Struct diff_e tells how many
 * items (diff_e.dist) one could encode with diff_e.cost bits starting
 * at index diff_e.idx. Note that the diff_e spans may overlap.
 * Fill_costs fills the diff_e list starting from pos-list end,
 * proceeding to its beginning. In the end the list is reversed.
 * In addition cost_s.max_cost will contain the minimum number of bits
 * which suffices to encode _any_ two subsequent items in the pos-list. 
 */
static const cost_s *fill_costs(const u32 *pos, uint len)
{
        static struct diff_e *diffs;
        static        uint   max_len;
        static        cost_s cost;
       
        int j = 0, i = len;
        
        if (len > max_len){
                if (diffs)
                        free(diffs);
                diffs      = (struct diff_e*)xmalloc(len * 
                                sizeof(struct diff_e));
                max_len    = len;
                cost.diffs = diffs;
        }

#if DEBUG
	if (!len)
		dub_die("len = 0 in fill_diffs()");
#endif
       
        cost.max_cost = bits_needed(pos[0]);
       
        while (i--){
                uint d;
          
                if (!i)
                        /* the first cost is pos[0] - 0 */
                        d = bits_needed(pos[0]);
                else
                        d = bits_needed(pos[i] - pos[i - 1]);
         
                #if DEBUG
                        debug_check(pos, i, len);
                #endif
                
                if (d > cost.max_cost)
                        cost.max_cost = d;
                
                /* The cost is same as the previous, just increase the
                 * run-length */
                if (j > 0 && d == cost.diffs[j - 1].cost){
                        cost.diffs[j - 1].idx = i;
                        ++cost.diffs[j - 1].dist;
                
                /* The cost is less, add a new run-length with initial
                 * length 1. */
                }else if (d < cost.diffs[j].cost){
                        cost.diffs[j].idx  = i;
                        cost.diffs[j].dist = 1;
                        cost.diffs[j].cost = d;
                        ++j;
               
                /* The cost is higher, let's see how many item we could
                 * encode with this many bits */
                }else{
                        uint k = j; 
                        cost.diffs[j].idx  = i;
                        cost.diffs[j].cost = d;
                        cost.diffs[j].dist = 1;
                      
                        while (k && d > cost.diffs[k].cost){
                                cost.diffs[j].dist = (cost.diffs[k].idx - i) +
                                                      cost.diffs[k].dist;
                                --k;
                        }
                        ++j;
                }
        }
      
        cost.nof_diffs = j;
        
        /* reverse the list */
        i = 0;
        while (i < --j){
                
                struct diff_e tmp = cost.diffs[j];
                cost.diffs[j]     = cost.diffs[i];
                cost.diffs[i]     = tmp;

                ++i;
        }

        return &cost;
}

static inline uint worst_cost(const cost_s *costs, uint len)
{
        return HEADER_COST + len * costs->max_cost +
               (len / MAX_ENTRIES_PER_SEGMENT) * HEADER_COST;
}

/* encoding_cost() estimates the number of bits needed to encode
 * the list using c_bits bits from idx to the end of the list. */
static inline uint encoding_cost(uint idx, uint *pos, uint c_bits, 
                                 const cost_s *costs, uint len)
{
        uint       cost        = HEADER_COST;
        uint       dist        = 0;
     
        /* find the next diff_e span */
        while (*pos < costs->nof_diffs - 1 && costs->diffs[*pos].idx < idx)
                ++*pos;
       
        /* c_bits is enough to cover the next span */
        if (costs->diffs[*pos].idx > idx && costs->diffs[*pos].cost <= c_bits){
                dist = costs->diffs[*pos].idx - idx;
                cost += dist * c_bits + (dist / MAX_ENTRIES_PER_SEGMENT) * 
                        HEADER_COST;
                dist += costs->diffs[*pos].dist;
                cost += costs->diffs[*pos].dist * c_bits +
                        (costs->diffs[*pos].dist / MAX_ENTRIES_PER_SEGMENT) *
                        HEADER_COST;

        /* c_bits suffices only for this span */
        }else{
                dist = costs->diffs[*pos].dist;
                cost += dist * c_bits + (dist / MAX_ENTRIES_PER_SEGMENT) * 
                        HEADER_COST;
        }
        
        /* the rest of the list is encoded with the maximum number of bits */
        if (dist < len - idx)
                cost += HEADER_COST + (len - idx - dist) * costs->max_cost +
                        ((len - idx - dist) / MAX_ENTRIES_PER_SEGMENT)
                        * HEADER_COST;

        return cost;
}


uint pc_encode(char *dest, u32 *offs, const u32 *pos, uint len)
{
        u32   o_offs       = 0;
        u32   prev         = 0;
        uint  c            = 0;
        uint  e_bits       = 0;
        uint  cur_cost     = 0;
        uint  cand_cost    = 0;
        u32   max_dif      = 0;
        uint  field_change = 1;
        u32   cnt_offs     = *offs;
        uint  pp           = 0;
        uint  i;
        
        /* special case: empty list, all zero */
        if (!len){
                *offs += 5;
                return 5;
        }

        const cost_s *costs = fill_costs(pos, len);
                
        o_offs = *offs;
                
        /* Encoding algorithm:
         * We start with the worst case cost estimate (cost in bits needed).
         * After each entry we evaluate whether changing the entry width
         * (e_bits) would result in a shorter code-length (cand_cost). If it
         * does, we change the width. The cost estimates are always worst
         * case estimates w.r.t the current position in the list. */

        cur_cost = worst_cost(costs, len);
        max_dif  = (1 << costs->max_cost) - 1;
        e_bits   = costs->max_cost;
        
        for (i = 0; i < len; i++){
                        
                u32  diff      = pos[i] - prev;
                uint c_bits    = bits_needed(diff);
                
                cand_cost = encoding_cost(i, &pp, c_bits, costs, len);
                
                if (max_dif < diff || cand_cost < cur_cost){
                        e_bits       = c_bits;
                        cur_cost     = cand_cost;
                        field_change = 1;
                        max_dif      = (1 << e_bits) - 1;
                }

                if (!c || field_change){
//                        dub_dbg("Field change to %u", e_bits);
                        if (i){
                                /* End of segment */
                                /* continuation flag: true */
                                uint header = ((MAX_ENTRIES_PER_SEGMENT - 1)
                                       - c) | (1 << NOF_ENTRIES_WIDTH);
                                /* write count of the previous run */
                                pc_write_bits16(dest, cnt_offs, header);

//                                dub_dbg("Offs %u Last seg %u", cnt_offs,
//                                (MAX_ENTRIES_PER_SEGMENT - c));
                        }
                        /* write header */
                        pc_write_bits16(dest, *offs, e_bits);
                        *offs += 5;
                        /* reserve space for continuation flag + nof_entries */
                        cnt_offs = *offs;
                        *offs += NOF_ENTRIES_WIDTH + 1;
                        field_change = 0;
                        c = MAX_ENTRIES_PER_SEGMENT;
                
                        cur_cost -= HEADER_COST;
                }
                
    //            dub_dbg("VAL %u DIFF %u %u of: %u", prev.gen + diff, 
     //           diff, e_bits, *offs);
                
                /* write entry */
                pc_write_bits32(dest, *offs, diff);

                cur_cost -= e_bits;
                *offs += e_bits;
                --c;

                prev = pos[i];
        }

        /* write end */
        /* no continuation: zero flag */
        pc_write_bits16(dest, cnt_offs, (MAX_ENTRIES_PER_SEGMENT - 1) - c);
        
        return *offs - o_offs;
}

uint pc_encode_signed(char *dest, u32 *offs, s32 *lst, uint len)
{
        int i;
        for (i = 0; i < len; i++)
                if (lst[i] >= 0)
                        lst[i] *= 2;
                else
                        lst[i] = 2 * -lst[i] + 1;

        return pc_encode(dest, offs, lst, len);
}

uint pc_decode(u32 **dest, uint *dest_len, const char *src, u32 *offs)
{
        uint  dest_left = *dest_len;
        uint  dest_cur  = 0;
        uint  nof_ent   = 0;
        uint  cont      = 1;

        u32 vals[MAX_ENTRIES_PER_SEGMENT];
        
        while (cont){
                
                read_header_x(vals, &cont, &nof_ent, src, offs);
                if (!nof_ent)
                        break;

                if (nof_ent > dest_left){

                        u32 *new_dest = (u32*)xmalloc(
                                                (*dest_len + DECODE_INC) * 4);
                        if (*dest){
                                memcpy(new_dest, *dest, *dest_len * 4);
                                free(*dest);
                        }
                        
                        *dest     = new_dest;
                        *dest_len += DECODE_INC;
                        dest_left += DECODE_INC;
                }

                dest_left -= nof_ent;
                
                memcpy(&((*dest)[dest_cur]), vals, nof_ent * 4);
                
                dest_cur += nof_ent;
        }
        
        return dest_cur;
}

#if 0
int main(int argc, char **argv)
{
        char dest[1024];
        u32 offs      = 0;
        u32  *de_dest = NULL;
        gen_t val     = (gen_t)0;
        uint dest_len = 0;
        
        gen_t pos[] = {1,10,11,12,100};
        
        int i;
        memset(&dest, 0, 1024);

        dub_init();
        
        const cost_s *s = fill_costs(pos, sizeof(pos) / 4);
       
        for (i = 0; i < s->nof_diffs; i++){
                printf("i[%u] idx %u dist %u cost %u\n", 
                        i, s->diffs[i].idx, s->diffs[i].dist, 
                        s->diffs[i].cost);
        }
        
        for (i = 0; i < sizeof(pos) / 4; i++){
                uint  d = bits_needed(pos[i].gen - pos[i - 1].gen);
                printf("i %u d %u Cost %u\n", 
                        i, d, encoding_cost(i, d, s, sizeof(pos) / 4));
        }
        
        
        uint bits = pc_encode(dest, &offs, pos, sizeof(pos) / 4);
        printf("Bits used: %u\n", bits);
//        write(STDOUT_FILENO, dest, (bits >> 3) + 1);

        offs = 0;

        printf("VALS: ");
        PC_FOREACH_VAL(dest, &offs, val)

                printf("%u ", val.gen);
                
        PC_FOREACH_VAL_END
        printf("\n");
        #if 0
        uint nof_ent = pc_decode(&de_dest, &dest_len, dest, &offs);
        fprintf(stderr,"DECODED: ");
        for (i = 0; i < nof_ent; i++)
                fprintf(stderr,"%u ", de_dest[i]);
        fprintf(stderr, "\n");
        #endif

}
#endif
