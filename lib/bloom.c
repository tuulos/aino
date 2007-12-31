/*
 *   lib/bloom.c
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


#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>

#include <dub.h>
#include <bloom.h>

#define LOG2_10 3.3219280948873626 /* log_2(10) */

/*
k = ceil(-log2(f))
n = ceil(log2(Ck))

f is the proportion of false hits at most
C is the number of items 
k is the number of hashes 
n determines the size of the bitvector, 2^n

*/
uint  _bloom_k; 
float _bloom_scaler;

inline uint ceil_log2(uint val)
{
        uint tst = val;
        uint log = 0;
        while((tst >>= 1)) ++log;

        if (1 << log < val)
                ++log;
        return log;
}

void init_bloom(uint f, float s)
{
        // The code below equals ceil(-log2(1E-f))
        _bloom_k = (uint)((float)f * LOG2_10);
        if (_bloom_k < ((float)f * LOG2_10))
                ++_bloom_k;

        _bloom_scaler = s;
        
        dub_dbg("Bloom K = %u scaler = %f", _bloom_k, s);
}

bloom_s *new_bloom(const u32 *lst, uint len)
{
        // The code below equals ceil(log2(len * _bloom_k))
        return new_bloom_fixn(lst, len, ceil_log2(len * _bloom_k));
}

bloom_s *new_bloom_fixn(const u32 *lst, uint len, uint n)
{
        bloom_s *blo    = NULL;
        u32     *idxs   = NULL;
        uint    max_idx = 0;
        uint    i;
        
        if (n > 32){
                dub_warn("Bloom of 2^n requested. Pruned to 2^32.", n);
                n = 32;
        }

        if (n < 3){
                blo  = xmalloc(1 + sizeof(bloom_s));
                memset(blo, 0, sizeof(bloom_s) + 1);
        }else{
                uint s = sizeof(bloom_s) + _bloom_scaler * (1 << (n - 3));
                blo    = xmalloc(s);
                memset(blo, 0, s);
        }

        idxs    = bloom_precomp_idxs(lst, len, n);
        blo->n  = n;
        max_idx = _bloom_scaler * (1 << blo->n);

        for (i = 0; i < len * _bloom_k; i++){

                if (idxs[i] > max_idx)
                        continue;

                blo->vec[idxs[i] >> 3] |= 1 << (idxs[i] & 7);
        }

        free(idxs);

        return blo;
}

u32 *bloom_precomp_idxs(const u32 *lst, uint len, uint n)
{
        uint i, k  = 0;
        u32  *idxs = xmalloc(len * _bloom_k * 4);
        u32  msk   = (1 << n) - 1;

        for (i = 0; i < len; i++){

                uint   j     = _bloom_k;
                u32    offs  = 32;
                u32    hash  = 0;

                /* lst[i] + 2 as the second seed is pure magic.
                 * It seems to work adequately. */
                rrand_seed(lst[i], lst[i] + 2);
        
                while (j--){
                
                        u32 idx;
                        if (offs + n > 32){
                                hash = rrand();
                                offs = 0;
                        }
                
                        idx = hash & msk;
                        hash >>= n;
                        offs += n;
                
                        idxs[k++] = idx;
                }
        }

        return idxs;
}
