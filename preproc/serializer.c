/*
 *   preproc/serializer.c
 *   serializes tokenized stream
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

#include <dub.h>

#include "serializer.h"

#define INITIAL_SIZE 500
#define out_f stdout
#define in_f  stdin

static growing_glist *lstbuf;
static const glist empty = {0};

void open_serializer()
{
        lstbuf = xmalloc(INITIAL_SIZE * 4 + sizeof(growing_glist));
        lstbuf->sze = INITIAL_SIZE;
        lstbuf->lst.len = 0;
}

void serialize_head(u32 id, const Pvoid_t head_bag)
{
        fwrite(&id, 4, 1, out_f);
        
        //dub_dbg("head");
        
        serialize_segment(head_bag);
}

void serialize_segment(const Pvoid_t segment_bag)
{
        int tst;
        Word_t idx = 0;
        
        J1C(tst, segment_bag, 0, -1);       
        fwrite(&tst, 4, 1, out_f);

        //dub_dbg("Seri %u", tst);
        
        J1F(tst, segment_bag, idx);
        while (tst){
                fwrite(&idx, 4, 1, out_f);
                J1N(tst, segment_bag, idx);
        }
}

const glist *pull_head(u32 *key)
{
        const glist *g = NULL;
        
        if (fread(key, 4, 1, in_f) < 1)
                return NULL;

        //dub_dbg("HEAD");
        
        if ((g = pull_segment()))
                return g;

        return &empty;
}

const glist *pull_segment()
{
        if (fread(&lstbuf->lst.len, 4, 1, in_f) < 1)
                dub_die("EOF in place of segment length");

        if (!lstbuf->lst.len)
                return NULL;
       
        //dub_dbg("LEN %u", lstbuf->lst.len);
        
        if (lstbuf->lst.len > lstbuf->sze){
                uint sze = lstbuf->sze + lstbuf->lst.len;
                lstbuf = realloc(lstbuf, sze * 4 + sizeof(growing_glist));
                if (!lstbuf)
                        dub_sysdie("Couldn't grow list buffer to %u items", 
                                        sze);
                lstbuf->sze = sze;
        }
        
        if (fread(&lstbuf->lst.lst, 4, lstbuf->lst.len, in_f) < 
                        lstbuf->lst.len)
                dub_die("Truncated segment");

        return &lstbuf->lst;
}

inline void push_head(u32 key, const glist *g)
{
        //dub_dbg("key %x head %u", key, g->len);
        fwrite(&key, 4, 1, out_f);
        fwrite(g, 4, g->len + 1, out_f);
}

inline void push_segment(const glist *g)
{
        if (g){
                //dub_dbg("G %u", g->len);
                fwrite(g, 4, g->len + 1, out_f);
        }else{
                //dub_dbg("G 0");
                fwrite(&empty, 4, 1, out_f);
        }
}



