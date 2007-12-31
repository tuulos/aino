/*
 *   lib/rrice.c
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

#include <stdlib.h>

#include "dub.h"
#include "rrice.h"

uint bits_needed(uint max_val)
{
        uint val = max_val;
        uint e_bits = 0;
        while (val){
                val >>= 1;
                ++e_bits;
        }
        return e_bits;
}

inline void elias_gamma_write(char *dest, u32 *offs, u32 val)
{
#if DEBUG
        if ((!val) || val >= 2147483648U)
                dub_die("Elias can't handle val=0 or val >= 2^31");
#endif
        uint b = bits_needed(val);
        if (!dest){
                *offs += 2 * b - 1;
                return;
        }
        int p = ffs(val);
        val >>= p - 1;
        *offs += b - 1;
        pc_write_bits32(dest, *offs, val);
        *offs += b;
}

inline void rice_write(char *dest, u32 *offs, u32 val, uint f)
{
#if DEBUG
        if (f >= 31)
                dub_die("Rice_write: f=%u >= 31", f);
        if ((val >> f) >= 31)
                dub_die("Rice_write: Value %u too large for f=%u",
                                val, f);
#endif
        pc_write_bits32(dest, *offs, 1 << (val >> f));
        *offs += 1 + (val >> f);
        pc_write_bits32(dest, *offs, val & ((1 << f) - 1));
        *offs += f;
}

uint estimate_rice_f(const u32 *lst, uint len)
{
        uint i, prev, max = 0;
        uint b = prev = 0;
        for (i = 0; i < len; i++){
                b += lst[i] - prev;
                if (lst[i] - prev > max)
                        max = lst[i] - prev;
                prev = lst[i];
                #if DEBUG
                        if (lst[i] == 4294967295)
                                dub_die("Invalid value: 2^31");
                #endif
        }
        /* This increment is crucial for performance! */
       	//++f;
        return estimate_rice_f_param(b, max, len);
}

uint estimate_rice_f_param(uint sum, uint max, uint len)
{
        uint f;
        if (!len)
                dub_die("Estimate_rice_f with an empty list");
        
        sum /= len;
        for (f = 0; (1 << f) <= sum + 0; f++);
                
        if ((max >> f) >= 31)
                for (; (max >> f) >= 31; f++);

        return f;
}

uint rice_encode(char *dest, u32 *offs, const u32 *lst, uint len)
{
        uint orig_offs = *offs;
       
        elias_gamma_write(dest, offs, len + 1);
        if (!len)
                return *offs - orig_offs;

        uint max, f, i, prev;
        /*
        for (max = 0, prev = 0, i = 0; i < len; i++){

                if (lst[i] - prev > max)
                        max = lst[i] - prev;
                prev = lst[i];
        }
        */

        f = estimate_rice_f(lst, len);
        rice_write(dest, offs, f, 2);
        
        for (prev = 0, i = 0; i < len; i++){
                rice_write(dest, offs, lst[i] - prev, f);
                prev = lst[i]; 
        }

        return *offs - orig_offs;
}

uint rice_encode_nolength(char *dest, u32 *offs, const u32 *lst, uint len)
{
        if (!len)
                return 0;

        uint orig_offs = *offs;
        uint f = estimate_rice_f(lst, len);
        rice_write(dest, offs, f, 2);
        
        uint i, prev = 0;
        for (i = 0; i < len; i++){
                rice_write(dest, offs, lst[i] - prev, f);
                prev = lst[i]; 
        }

        return *offs - orig_offs;
}

uint rice_encode_signed(char *dest, u32 *offs, s32 *lst, uint len)
{
        int i;
        for (i = 0; i < len; i++){
                if (lst[i] >= 0)
                        lst[i] *= 2;
                else
                        lst[i] = 2 * -lst[i] + 1;
        }

        return rice_encode(dest, offs, (u32*)lst, len);
}

uint rice_decode(u32 **dest, uint *dest_len, const char *src, u32 *offs)
{
        uint len = elias_gamma_read(src, offs) - 1;
        if (!len)
                return 0;

        if (*dest_len < len){
                *dest = realloc(*dest, (*dest_len + len + DECODE_INC) * 4);
                *dest_len = *dest_len + DECODE_INC + len;
        }
        uint f = rice_read(src, offs, 2);
        uint cum = 0;
        uint i = 0;
        while (len--){
                cum += rice_read(src, offs, f);
        	(*dest)[i++] = cum;
        }
        
        return i;
}

uint rice_decode_signed(s32 **dest, uint *dest_len, const char *src, u32 *offs)
{
        int i, len = rice_decode((u32**)dest, dest_len, src, offs);
        
        for (i = 0; i < len; i++)
                if ((*dest)[i] & 1)
                        (*dest)[i] = ((*dest)[i] - 1) / -2;
                else
                        (*dest)[i] /= 2;

        return len;
}
