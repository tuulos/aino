/*
 *   lib/bitop.h
 *   Bit tweaking
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
 */


#ifndef __BITOP_H__
#define __BITOP_H__

#include <ttypes.h>

#if __WORDSIZE == 64
        #define __BIT_DV 6
#else
        #define __BIT_DV 5
#endif
#define __BIT_MD (__WORDSIZE - 1)

static char bits_in_16bits[1 << 16];
static void init_bitcount()
{
        uint i;
        for (i = 0; i < (1 << 16); i++){
                uint count = 0, n = i;
                while (n){
                        ++count;
                        n &= (n - 1);
                }
                bits_in_16bits[i] = count;
        }
}

#if __WORDSIZE == 64

static inline uint bitcount (unsigned long n)
{
     return bits_in_16bits [n         & 0xffffu]
         +  bits_in_16bits [(n >> 16) & 0xffffu]
         +  bits_in_16bits [(n >> 32) & 0xffffu]
         +  bits_in_16bits [(n >> 48) & 0xffffu];
}

#else

static inline uint bitcount (unsigned long n)
{
     return bits_in_16bits [n         & 0xffffu]
         +  bits_in_16bits [(n >> 16) & 0xffffu];
}

#endif

/* asm_ functions are stolen from the kernel source at asm/bitops.h.
 * Works only with i386. */

#define ASM_MCLEAR_BIT(m, x) asm_clear_bit(x & __BIT_MD, &m[x >> __BIT_DV])

static inline void asm_clear_bit(int nr, volatile unsigned long *addr)
{
        __asm__ __volatile__(
                "btrl %1,%0"
                :"=m" ((*(volatile long*) addr))
                :"Ir" (nr));
}

#define ASM_MSET_BIT(m, x) asm_set_bit(x & __BIT_MD, &m[x >> __BIT_DV])

static inline void asm_set_bit(int nr, volatile unsigned long *addr)
{
        __asm__(
                "btsl %1,%0"
                :"=m" ((*(volatile long*) addr))
                :"Ir" (nr));
}

#define ASM_MTEST_BIT(m, x) asm_test_bit(x & __BIT_MD, &m[x >> __BIT_DV])

static inline int asm_test_bit(int nr, const volatile unsigned long *addr)
{
        int oldbit;

        __asm__ __volatile__(
                "btl %2,%1\n\tsbbl %0,%0"
                :"=r" (oldbit)
                :"m"  ((*(volatile long*) addr)), "Ir" (nr));
        return oldbit;
}


/*
static inline unsigned long asm_ffz(unsigned long word)
{
        __asm__("bsfl %1,%0"
                :"=r" (word)
                :"r" (~word));
        return word;
}

static inline unsigned long asm_ffs(unsigned long word)
{
        __asm__("bsfl %1,%0"
                :"=r" (word)
                :"rm" (word));
        return word;
}
*/


#endif /* __BITOP_H__ */
