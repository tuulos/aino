/*
 *   preproc/qexp_tagger.c
 *   Expand ixemes of a document using a QEXP table
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



#include <qexpansion.h>
#include <mmisc.h>
#include <pparam.h>
#include <ttypes.h>
#include <dub.h>
#include "serializer.h"


int main(int argc, char **argv)
{
        glist  *ix_meta = NULL;
        glist  *ix_tok  = NULL;
        glist  **pos    = NULL;
        u32    key      = 0;

        const char *qexp_file = NULL;
        
        dub_init();

        qexp_file = pparm_common_name("qexp");
        
        load_qexp(qexp_file, 0);

        open_serializer();
        
        while (deserialize_doc(&key, &ix_meta, &ix_tok, &pos)){
                uint  i;
                glist *merged = NULL;
                
                for (i = 0; i < ix_meta->len.gen; i++){
                        
                        const glist *exp = get_expansion(-ix_meta->lst[i].ixid);
                        if (!exp)
                                continue;

                        if (merged){
                                glist *old = merged;
                                merged = merge_glists(merged, exp);
                                free(old);
                        }else
                                merged = merge_glists(ix_meta, exp);
                }

                for (i = 0; i < ix_tok->len.gen; i++){
                        
                        const glist *exp = get_expansion(ix_tok->lst[i].ixid);
                        if (!exp)
                                continue;

                        if (merged){
                                glist *old = merged;
                                merged = merge_glists(merged, exp);
                                free(old);
                        }else
                                merged = merge_glists(ix_meta, exp);
                }
        
                if (merged)
                        serialize_doc(key, merged, ix_tok, pos);
                else
                        serialize_doc(key, ix_meta, ix_tok, pos);
        }

        close_serializer();
        
        return 0;
}
