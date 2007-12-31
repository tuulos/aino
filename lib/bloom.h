/*
 *   lib/bloom.h
 *   Creates a bloom filter for a list
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

#ifndef __BLOOM_H__
#define __BLOOM_H__

#include <ttypes.h>
#include <rrand.h>
#include <pcode.h>

extern uint  _bloom_k;
extern float _bloom_scaler;

typedef struct{
        unsigned char n;
        char          vec[0];
} __attribute__((packed)) bloom_s;

#define SIZEOF_BLOOM(b) (sizeof(bloom_s) + ((b)->n < 3 ? 1:\
                        _bloom_scaler * (1 << ((b)->n - 3))))

#define MATCH_BLOOM_IDX(b, i) (b->vec[i >> 3] & (1 << (i & 7)))

static inline uint bloom_test(const bloom_s *b, u32 idx)
{
        uint  i       = _bloom_k;
        u32   offs    = 32;
        u32   hash    = 0;
        uint  max_idx = 0;
        
        uint msk = (1 << b->n) - 1;

        max_idx = _bloom_scaler * (1 << b->n);
        
        rrand_seed(idx, idx + 2);
        
        while (i--){
                u32 idx = 0;
                if (offs + b->n > 32){
                        hash = rrand();
                        offs = 0;
                }
                
                idx = hash & msk;
                hash >>= b->n;
                offs += b->n;

                if (idx > max_idx) continue;
                
                if (!MATCH_BLOOM_IDX(b, idx))
                        return 0;
        }

        return 1;
}

/* f: allow at most one false hit in 10^f queries */
/* s: bloom scaler. Size percentage w.r.t the optimal */
void    init_bloom(uint f, float s);
bloom_s *new_bloom(const u32 *lst, uint len);
bloom_s *new_bloom_fixn(const u32 *lst, uint len, uint n);
u32     *bloom_precomp_idxs(const u32 *lst, uint len, uint n);
                
        
#endif /* __BLOOM_H__ */


        
