/*
 *   preproc/tok.l
 *   Flex rules for tokenizer
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

%option noyywrap

LETTER [[:alpha:]\xc0-\xff]

%{
#include <string.h>
#include <ctype.h>

#include <dub.h>
#include <gglist.h>
#include <ixemes.h>

#include "serializer.h"
#include "tok.h"
#include "encode_pos.h"

#define YY_USER_ACTION {\
        uint i = yyleng;\
        while(i--) yytext[i] = tolower(yytext[i]);\
}
%}

%%
          cur_pos     = 0;
          token_cnt   = 1;
          seg_cnt     = 0;
          segment_bag = NULL;
          
          yy_scan_string(doc_buf);
                          
 /* skip html entities */
&[[:alpha:]]+;  { cur_pos += yyleng; }

 /* Allow only tokens that begin with a valid letter.
    Pure digits are not allowed. */
({LETTER})+[[:alnum:]]*   { 

        if (yyleng < max_token_len){

                Word_t *ix_id = NULL;
                
                if (ixicon_read_only){
                        JSLG(ix_id, ixicon, yytext);
                }else{
                        JSLI(ix_id, ixicon, yytext);
                        if (!*ix_id){
                                *ix_id = xid++;

                                if (xid > XID_TOKEN_L)
                                        dub_die("Ran out of XID_TOKEN IDs. "\
                                        "Increase XID_TOKEN range.");
                        }
                }
                        
                if (ix_id && *ix_id){ 

                        int           tst;
                        Word_t        *ptr; 
                        growing_glist *g;

                        J1S(tst, segment_bag, *ix_id);

                        if (ixicon_read_only){
                                JLI(ptr, tokens, *ix_id);
                                if (!*ptr){
                                        GGLIST_INIT(g, 5);
                                }else
                                        g = (growing_glist*)*ptr;        
                        
                                GGLIST_APPEND(g, token_cnt);
                                *ptr = (Word_t)g;
                
                                //GGLIST_APPEND(token_positions, cur_pos);
                                
                                ++token_cnt;
                        }
                        if (++seg_cnt == segment_size){
                                serialize_segment(segment_bag);
                                J1FA(tst, segment_bag);
                                seg_cnt = 0;
                                
                                if (ixicon_read_only){
                                        possect_add(tokens);
                                        JLFA(tst, tokens);
                                        tokens = NULL;
                                        token_cnt = 1;
                                }
                        }
                }
        }
        
        cur_pos += yyleng;
}
                          
.|\n    { cur_pos += yyleng; }

<<EOF>> { 
        int tst;

        if (seg_cnt){
                serialize_segment(segment_bag);
                J1FA(tst, segment_bag);
                if (ixicon_read_only){
                        possect_add(tokens);
                        JLFA(tst, tokens);
                }
        }

        yy_delete_buffer(YY_CURRENT_BUFFER); 
        yyterminate(); 
        }
%%

