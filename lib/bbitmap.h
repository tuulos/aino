/*
 *   lib/bbitmap.h
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


#ifndef __BBITMAP_H__
#define __BBITMAP_H__

#include <dub.h>
#include <bitop.h>
#include <string.h>

int ffsl(long int i);

#if __WORDSIZE == 64
        #define DV 6
#else
        #define DV 5
#endif
#define MD (__WORDSIZE - 1)

typedef struct{
        long len;
        unsigned long b[0];
} bmap_t;

static bmap_t *bmap_init(uint size)
{
        bmap_t *b;
        uint sze;

        if (size & MD)
                sze = ((size >> DV) + 1);
        else
                sze = (size >> DV);

        dub_msg("SIXR %u", sze);
        b = xmalloc((sze + 1) * sizeof(long));
        b->len = size;
        memset(b->b, 0, sze * sizeof(long));
        return b;
}

/*
static inline bmap_t *bmap_clone(const bmap_t *bmap)
{
        bmap_t *b = xmalloc(sizeof(bmap_t) + bmap->len * sizeof(long));
        memcpy(b, bmap, sizeof(bmap_t) + bmap->len * sizeof(long));
        return b;
}
*/

static inline int bmap_test(const bmap_t *bmap, uint idx)
{
	if (idx >= bmap->len)
		return 0;
	else
	        return asm_test_bit(idx & MD, &bmap->b[idx >> DV]);
}

static inline void bmap_set(bmap_t *bmap, uint idx)
{
        asm_set_bit(idx & MD, &bmap->b[idx >> DV]);
}

static inline void bmap_unset(bmap_t *bmap, uint idx)
{
        asm_clear_bit(idx & MD, &bmap->b[idx >> DV]);
}

static inline uint bmap_first_set(const bmap_t *bmap)
{
        uint i, r;
        for (i = 0; i < bmap->len; i++)
                if ((r = ffsl(bmap->b[i])))
                        return r + (i << DV);
        return 0;
}

static inline uint bmap_first_unset(const bmap_t *bmap)
{
        uint i, r;
        for (i = 0; i < bmap->len; i++)
                if ((r = ffsl(~bmap->b[i])))
                        return r + (i << DV);
        return 0;
}



#endif /* __BBITMAP_H__ */
