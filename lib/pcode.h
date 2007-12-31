/*
 *   lib/pcode.h
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
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _PCODE_H_
#define _PCODE_H_

#include "ttypes.h"

#define MAX_ENTRIES_PER_SEGMENT 16

/* NB: NOF_ENTRIES_WIDTH + 6 must be less than 16! */
#define NOF_ENTRIES_WIDTH   4  /* = log_2(MAX_ENTRIES_PER_SEGMENT) */

/* In the worst case encoding puts one item per segment. Thus there's
 * the header (5 + NOF_ENTRIES_WIDTH + 1) bits (+ 7 bits in case that
 * the header isn't byte aligned) and the entry itself, taking at most
 * 32 bits. The final + 8 makes sure that the reading-window stays inside. */
#define ENCODE_BUFFER_SIZE(i) ((((i) * (NOF_ENTRIES_WIDTH + 6 + 39)) >> 3) + 8)


#define TYPE long
static inline void expand_bits(u32 *dst, const char *src, u32 *offs,
                               u32 prev, uint ent_wdt, uint nof_ent)
{
        uint        i          = 0;
        TYPE        e_msk      = (1 << ent_wdt) - 1;
        
        while (i < nof_ent){

                uint        sft   = *offs & 7;
                TYPE        cur   = (TYPE)((*(u64*)&src[*offs >> 3]) >> sft);
                uint        toff  = 0; 
                uint        max   = (sizeof(TYPE) << 3) - sft;

                while (i < nof_ent){
                        dst[i++] = prev = prev + (cur & e_msk);
                        cur >>= ent_wdt;
                        toff += ent_wdt;

                        if (toff + ent_wdt > max)
                                break;
                }
                *offs += toff;
        }
}

/* Reads len values from lst and writes the corresponding
 * pcode-list to dest. Returns the number of bits written
 * and increases offs accordingly. */

/* 1. CRUCIAL ASSUMPTION: lst items MUST be in ascending order and not empty 
 * 2. CRUCIAL ASSUMPTION: all entries > 0 
 * 3. CRUCIAL ASSUMPTION: dest is initialized to zero */
uint pc_encode(char *dest, u32 *offs, const u32 *lst, uint len);

/* Reads a pcoded list from src starting at offs and writes the decoded
 * list to dest. If the list is longer than dest_len, dest is grown and
 * dest_len increased accordingly. Offs is increased by the number of
 * bits read. Returns the number of entries (NB. dest_len >= #entries).
 *
 * You may set *dest = NULL and dest_len = 0 initially.
 * */
uint pc_decode(u32 **dest, uint *dest_len, const char *src, u32 *offs);

/*
uint pc_fast_forward(const char *dat, uint idx, u32 *offs);
uint pc_isect(gen_t **stack, const char *dat1, const char *dat2,
              u32 *offs1, u32 *offs2, uint mode);
uint pc_isect1(gen_t **stack, const char *dat1, const char *dat2,
              u32 *offs1, u32 *offs2, uint mode);
uint pc_index(gen_t val, const char *dat, u32 offs);
*/

void pc_print(const char *dat, u32 offs);

static inline uint pc_log2(uint val)
{
        uint log = 0;
        while((val >>= 1)) ++log;
        return log;
}

/* WARNING! Strictly speaking the following pc_read and pc_write functions are
 * not safe. They should never corrupt data but they may cause a segfault under
 * some circumstances -- namely when you're writing last bits of an array and
 * the functions reference a slice which goes over the array boundary due to use
 * of over-sized window for operations. 
 *
 * This is not a problem if you reserve 7 extra zero bytes in the end of the
 * array.
 * 
 * TODO: Less braindead bit operations
 * 
 */
static inline void pc_read_bits16(u16 *dst, const char *src, u32 offs, uint bits)
{
        const uint  sft  = offs & 7;
        const u32 *src_w = (u32*)&src[offs >> 3];
        const u32 msk    = (1 << bits) - 1;
        
        *dst = ((*src_w) >> sft) & msk;
}

static inline void pc_read_bits32(u32 *dst, const char *src, u32 offs, uint bits)
{
        const uint  sft    = offs & 7;
        const u64 *src_w = (u64*)&src[offs >> 3];
        const u32 msk    = (1 << bits) - 1;
        
        *dst = ((*src_w) >> sft) & msk;
}

static inline void pc_write_bits16(char* dst, u32 offs, u32 val)
{
        if (dst){
                const uint  sft    = offs & 7;
                u32         *dst_w = (u32*)&dst[offs >> 3];
        
                *dst_w |= val << sft;
        }
}

static inline void pc_write_bits32(char* dst, u32 offs, u32 val)
{
        if (dst){
                const uint  sft    = offs & 7;
                u64         *dst_w = (u64*)&dst[offs >> 3];
        
                *dst_w |= ((u64)val) << sft;
        }
}

#define ENTRY_WIDTH(x) (x & 0x1F)
#define NOF_ENTRIES(x) ((x >> 0x5) & ((1 << NOF_ENTRIES_WIDTH) - 1))
#define CONT_FLAG(x)   (x & (1 << (NOF_ENTRIES_WIDTH + 5)))

static inline uint read_header(const char *dat, u32 *offs, uint *nof_ent,
                                uint *ent_wdt, uint *cont)
{
        u16 header = 0;
        pc_read_bits16(&header, dat, *offs, 6 + NOF_ENTRIES_WIDTH);
        
        *nof_ent = NOF_ENTRIES(header) + 1;
        *ent_wdt = ENTRY_WIDTH(header);
        
        if (!*ent_wdt){
                *offs += 5;
                return 0;
        }else{
                *offs += 6 + NOF_ENTRIES_WIDTH;
                *cont = CONT_FLAG(header);
                return 1;
        }
}

static inline void read_header_x(u32 *vals, uint *cont, uint *nof_ent,
                                const char *dat, u32 *offs)
{
        u16  header  = 0;
        uint ent_wdt = 0;
        u32  prev    = *nof_ent == 0 ? 0: vals[*nof_ent - 1];
       
        pc_read_bits16(&header, dat, *offs, NOF_ENTRIES_WIDTH + 6);
       
//        dub_dbg("HED %d", *(char*)&header);
        
        *nof_ent = NOF_ENTRIES(header) + 1;
        ent_wdt  = ENTRY_WIDTH(header);
        
        if (!ent_wdt){
                *offs    += 5;
                *nof_ent = 0;
        }else{
                *offs += NOF_ENTRIES_WIDTH + 6;
                *cont = CONT_FLAG(header);
                expand_bits(vals, dat, offs, prev, ent_wdt, *nof_ent);
        }
        
      //  dub_dbg("PREC %u ent_w %u nof_ent %u cont %u offs %u", prev.gen, ent_wdt, 
        //                *nof_ent, *cont, *offs);
}



/* dat must be (const char*), offs (*u32) and val (u32) */
#define PC_FOREACH_VAL(dat, offs, val) {\
        u32  __vals[MAX_ENTRIES_PER_SEGMENT];\
        uint __cont    = 1;\
        uint __nof_ent = 0;\
        uint __i       = 0;\
        while (__cont){\
                read_header_x(__vals, &__cont, &__nof_ent, dat, offs);\
                if (!__nof_ent)\
                        break;\
                for (__i = 0; __i < __nof_ent; __i++){\
                        val = __vals[__i];
                        
#define PC_FOREACH_VAL_END }}}

#endif /* PCODE_H */
