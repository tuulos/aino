/*
 *   preproc/collect_stats.c
 *   Collect statistics on ixeme occurrences
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


#include <string.h>
#include <stdlib.h>

#include <Judy.h>

#include <dub.h>
#include <pparam.h>
#include <ixemes.h>
#include <ttypes.h>

#include "serializer.h"
#include "ixeme_stats.h"


static inline u32 lang_code(const glist *head)
{
        uint i;
        for (i = 0; i < head->len; i++)
                if (head->lst[i] < XID_META_LANG_F)
                        continue;
                else if (head->lst[i] > XID_META_LANG_L)
                        break;
                else
                        return head->lst[i];
        return 0;
}

int main(int argc, char **argv)
{
        const glist *seg = NULL;
        u32         key  = 0;

        Pvoid_t entries = NULL;
        
        uint   dc       = 0;   
        uint   ic       = 0;

        const char *stats = NULL;

        dub_init();
        
        stats = pparm_common_name("istats");
        
        open_serializer();

        while ((seg = pull_head(&key))){
        
                uint i;
                u32 lang = lang_code(seg);
                
                do{
                        for (i = 0; i < seg->len; i++){
                                
                                Word_t            *e    = NULL;
                                Word_t            idx   = seg->lst[i];
                                struct istat_entry *ent = NULL;
                                
                                JLI(e, entries, idx);
                                if (!*e){
                                        *e  = (Word_t)xmalloc(
                                                sizeof(struct istat_entry));
                                        ent = (struct istat_entry*)*e;
                                        memset(ent, 0, 
                                                sizeof(struct istat_entry));
                                        ent->xid = idx;
                                }                         

                                ent = (struct istat_entry*)*e;

                                ++ent->freq;
                                if (!ent->lang_code)
                                        ent->lang_code = lang;
                        }
                        
                }while ((seg = pull_segment()));

                ++dc;
        }
 
        ic = write_istats(stats, entries);
        
        dub_msg("Number of documents: %u\n", dc);
        dub_msg("Number of ixemes: %u\n", ic);        

        return 0;
}
