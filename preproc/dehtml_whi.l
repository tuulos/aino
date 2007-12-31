/*
 *   preproc/dehtml_whi.l
 *   Flex rules for whitespace normalization after dehtmlization
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


%option noyywrap

%{
#include "dehtml.h"
%}

%x intext
%%
                          yy_scan_string(doc_buf);

   /* jump to a paragraph */
[^[:space:]]              { BEGIN(intext); 
                            out_buf[out_len++] = yytext[0];
                           }

   /* jump out from the paragraph if the line is empty */
<intext>^[[:space:]]*[\n] { BEGIN(INITIAL); 
                            out_buf[out_len++] = '\n';
                          }
<intext>[ \t]+            { out_buf[out_len++] = ' '; }

   /* all whitespace outside the paragraphs is collapsed to a newline */
[[:space:]]+              { out_buf[out_len++] = '\n'; }

   /* default rule */
<INITIAL,intext>(.|\n)    { out_buf[out_len++] = yytext[0]; }

<<EOF>>   { yy_delete_buffer(YY_CURRENT_BUFFER); 
            yyterminate(); 
           }
