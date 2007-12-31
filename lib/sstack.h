/*
 *   lib/sstack.h
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


#ifndef __SSTACK_H__
#define __SSTACK_H__

#include <dub.h>
#include <gglist.h>

typedef growing_glist sstack_t;

static sstack_t *sstack_new()
{
        growing_glist *s;
        GGLIST_INIT(s, 100);
        return s;
}

static inline void sstack_push(sstack_t **s, u32 item)
{
#ifdef DEBUG
        if (!item)
                dub_die("Can't push 0 to stack");
#endif
        GGLIST_APPEND((*s), item);
}

static inline u32 sstack_pop(sstack_t *s)
{
        if (s->lst.len)
                return s->lst.lst[--s->lst.len];
        else
                return 0;
}


#endif /* __SSTACK_H__ */
