/*
 *   lib/gglist.h
 *   More or less dynamic arrays
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

#ifndef __GGLIST_H__
#define __GGLIST_H__

#include <ttypes.h>

typedef struct{
        u32 len;
        u32 lst[0];
} glist;

typedef struct{
        uint  sze;
        glist lst;
} growing_glist;

#ifndef _DUB_H_

        /* This makes possible to use this header independently
         * from the rest of libaino. At least required by AinoDump. */

        #include <stdarg.h>
        #include <stdio.h>
        #include <stdlib.h>

        static void dub_sysdie(const char *fmt, ...)
        {
                va_list ap;
                vfprintf(stderr, fmt, ap);
                exit(1);
                va_end(ap);
        }
        
        static void *xmalloc(uint size)
        {
                void *p = malloc(size);
                if (!p)
                       dub_sysdie("Memory allocation failed (%u bytes)", size);
                return p;
        }
#endif

#ifndef GGINC
#define GGINC 256
#endif

#define GGLIST_INIT(g, l) {\
        g = xmalloc((l) * 4 + sizeof(growing_glist));\
        g->sze = (l); g->lst.len = 0;\
}

#define GGLIST_APPEND(gg, val){\
        if (gg->lst.len == gg->sze){\
                uint sze = gg->sze + GGINC;\
                gg = realloc(gg, sze * 4 + sizeof(growing_glist));\
                if (!gg) dub_sysdie("Couldn't grow list to %u items", sze);\
                gg->sze = sze;\
        }\
        gg->lst.lst[gg->lst.len++] = val;\
}

#endif /* __GGLIST_H__ */
