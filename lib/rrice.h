/*
 *   lib/rrice.h
 *   
 *   Copyright (C) 2006-2008 Ville H. Tuulos
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


#ifndef __RICE_H__
#define __RICE_H__

#include <string.h>

#include "dub.h"
#include "ttypes.h"
//#include "pcode.h"

#define DECODE_INC 1024

#define BITS_TO_BYTES(bits) (((bits) >> 3) + 1)

uint rice_encode(char *dest, u32 *offs, const u32 *lst, uint len);
uint rice_encode_signed(char *dest, u32 *offs, s32 *lst, uint len);

uint rice_decode(u32 **dest, uint *dest_len, const char *src, u32 *offs);
uint rice_encode_nolength(char *dest, u32 *offs, const u32 *lst, uint len);
uint rice_decode_signed(s32 **dest, uint *dest_len, const char *src, u32 *offs);
uint estimate_rice_f(const u32 *lst, uint len);
uint estimate_rice_f_param(uint sum, uint max, uint len);

inline void elias_gamma_write(char *dest, u32 *offs, u32 val);
inline void rice_write(char *dest, u32 *offs, u32 val, uint f);

static inline void pc_write_bits32(char* dst, u32 offs, u32 val)
{
        if (dst){
                const uint  sft    = offs & 7;
                u64         *dst_w = (u64*)&dst[offs >> 3];
        
                *dst_w |= ((u64)val) << sft;
        }
}

static inline void pc_read_bits32(u32 *dst, const char *src, 
                                  u32 offs, uint bits)
{
        const uint  sft    = offs & 7;
        const u64 *src_w = (u64*)&src[offs >> 3];
        const u32 msk    = (1 << bits) - 1;
        
        *dst = ((*src_w) >> sft) & msk;
}


static inline u32 elias_gamma_read(const char *src, u32 *offs)
{
	
        u32 val;
        pc_read_bits32(&val, src, *offs, 31);
        int r, p = ffs(val);
#if DEBUG
        if (!p)
                dub_die("Invalid elias value");
#endif
        *offs += p - 1;
        pc_read_bits32(&val, src, *offs, p);
        *offs += p;
	
	__asm__("bsrl %1,%0"
                :"=r" (r)
                :"r" (val));
        
        return val << (p - (r + 1));
}

static inline u32 rice_read(const char *src, u32 *offs, uint f)
{
	u32 val;
	u32 p = *offs;
	__asm__(
		"1:\tbtl %0,%1\n\t"
		"inc %0\n\t"
		"jnc 1b"
		:"=r" (p)
		:"m" (*src), "0" (p));

	p -= *offs;
	*offs += p;
        pc_read_bits32(&val, src, *offs, f);
        *offs += f;

	return val + (1 << f) * (p - 1);
}

#define DEX_FOREACH_VAL(ptr, offs, val){\
        uint __len = elias_gamma_read(ptr, offs) - 1;\
        if (__len){\
                uint __f = rice_read(ptr, offs, 2);\
                val = 0;\
                while (__len--){\
                        val += rice_read(ptr, offs, __f);

#define DEX_FOREACH_VAL_END }}}

#define INVA_FOREACH_VAL(e, offs, val){\
        uint __len = e->len;\
        uint __f = rice_read(e->inva, offs, 2);\
        val = 0;\
        while (__len--){\
                val += rice_read(e->inva, offs, __f);

#define INVA_FOREACH_VAL_END }}
        
#define POS_FOREACH_VAL(ptr, offs, val){\
        uint __f = rice_read(ptr, offs, 2);\
        while (1){\
                val = rice_read(ptr, offs, __f);\
                if (!val)\
                        break;

#define POS_FOREACH_VAL_END }}







                                       

        


#endif /* __RICE_H__ */
