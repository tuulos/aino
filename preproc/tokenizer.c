/*
 *   preproc/tokenizer.c
 *   tokenizes raw text stream
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

#include <unistd.h>
#include <locale.h>

#include <Judy.h>

#include <dub.h>
#include <pparam.h>
#include <ixicon.h>
#include <ixemes.h>

#include "rawstream.h"
#include "encode_pos.h"
#include "serializer.h"
#include "iblock.h"

#define MAX_TOKEN_LENGTH (MAX_TOK_LEN - 1)

int toklex();

const char    *doc_buf;

Pvoid_t       tokens;
//growing_glist *token_positions;

Pvoid_t       ixicon;
u32           xid;

uint          segment_size;
uint          ixicon_read_only;
uint          max_token_len;

int main(int argc, char **argv)
{
        struct rawdoc *doc;
        uint          doc_cnt = 0;
        const char    *ixname = pparm_common_name("ixi");
        
        dub_init();
        
        PPARM_INT(max_token_len, MAX_TOKEN_LENGTH);
        if (max_token_len >= MAX_TOK_LEN)
                dub_die("MAX_TOKEN_LENGTH must be < %u", MAX_TOK_LEN);
        
        PPARM_INT_D(segment_size, SEGMENT_SIZE);
        if (!segment_size)
                dub_die("SEGMENT_SIZE must be > 0");
        
        if (getenv("LOCALE")){
                if(!setlocale(LC_ALL, getenv("LOCALE")))
                        dub_sysdie("Couldn't set locale %s",
                                    getenv("LOCALE"));
                else
                        dub_msg("Locale set to %s", getenv("LOCALE"));
        }
        
        /* start from the scratch? */
        if (getenv("NEW_IXICON")) {
                dub_msg("Tokenizer starting with an empty ixicon.");
                ixicon = NULL;
                ixicon_read_only = 0;
        }else{
                dub_msg("Tokenizer will use a pre-made ixicon %s", ixname);
                load_ixicon(ixname);
                ixicon = judify_ixicon();
                ixicon_read_only = 1;
        }
        
        /* Tokenizer doesn't mark tokens as frequent, thus
         * all the IDs should come after XID_TOKEN_FREQUENT_L. */
        xid = XID_TOKEN_FREQUENT_L + 1;
        //GGLIST_INIT(token_positions, 500);
        
        if (ixicon_read_only)
                open_possect();
        
        while ((doc = pull_document())){

                char    *keystr  = head_get_field(doc->head, "ID");
                Pvoid_t meta_bag = head_meta_bag(doc->head);
                u32     key;
                int     tst;
                
                if (!keystr)
                        dub_die("Missing ID in %uth document.", doc_cnt + 1);
                
                errno = 0;
                key = strtoul(keystr, NULL, 10);
                if (errno)
                        dub_die("Something wrong in %uth document ID \"%s\"",
                                doc_cnt + 1, keystr);
                
                serialize_head(key, meta_bag);
                J1FA(tst, meta_bag);
                
                //token_positions->lst.len = 0;
                tokens = NULL;
                doc_buf = doc->body;
       
                if (ixicon_read_only){
                        ENSURE_IBLOCK
                        possect_add(tokens);
                }
                
                
                /* run tokenizer */
                toklex();

                /* mark document end */
                serialize_segment(NULL);
               
                /*
                if (ixicon_read_only){
                        possect_add(key, tokens);
                        JLFA(tst, tokens);
                }
                */
		
		if (!(++doc_cnt % 1000))
			dub_msg("%u documents tokenized", doc_cnt);

                free(keystr);
                free(doc->head);
        }

        if (ixicon_read_only)
                close_possect();
        else
                create_ixicon(ixname, ixicon);
        
        dub_msg("Done");

        return 0;
}
  

