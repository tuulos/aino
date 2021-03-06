/*
 *   preproc/dehtml.l
 *   Flex rules for entity expansion after dehtmlization
 *   
 *   Copyright (C) 2005-2008 Ville H. Tuulos
 *
 *   Based on Kimmo Valtonen's groundbreaking work with HTML2txt.pm.
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

HEX [[:digit:]ABCDEF]+

%{
#include "dehtml.h"
%}
%%
                yy_scan_string(doc_buf);

 /* Escape angles */
[<]     	{memcpy(&out_buf[out_len], "&gt;", 4); out_len += 4;} 
[>]    	        {memcpy(&out_buf[out_len], "&lt;", 4); out_len += 4;} 

 /* Entity expansions */
&"nbsp";        {out_buf[out_len++] =' ';}
&"quot";   	{out_buf[out_len++] ='"';} 
&"apos";   	{out_buf[out_len++] ='\'';} 
&"amp";   	{out_buf[out_len++] ='&';} 
&"copy";   	{out_buf[out_len++] ='�';} 
&"AElig";	{out_buf[out_len++] ='�';} 
&"Aacute";	{out_buf[out_len++] ='�';} 
&"Acirc";	{out_buf[out_len++] ='�';} 
&"Agrave";	{out_buf[out_len++] ='�';} 
&"Aring";	{out_buf[out_len++] ='�';} 
&"Atilde";	{out_buf[out_len++] ='�';} 
&"Auml";	{out_buf[out_len++] ='�';} 
&"Ccedil";	{out_buf[out_len++] ='�';} 
&"ETH";	        {out_buf[out_len++] ='�';} 
&"Eacute";	{out_buf[out_len++] ='�';}  
&"Ecirc";	{out_buf[out_len++] ='�';} 
&"Egrave";	{out_buf[out_len++] ='�';} 
&"Euml";	{out_buf[out_len++] ='�';} 
&"Iacute";	{out_buf[out_len++] ='�';} 
&"Icirc";	{out_buf[out_len++] ='�';} 
&"Igrave";	{out_buf[out_len++] ='�';} 
&"Iuml";	{out_buf[out_len++] ='�';} 
&"Ntilde";	{out_buf[out_len++] ='�';} 
&"Oacute";	{out_buf[out_len++] ='�';} 
&"Ocirc";	{out_buf[out_len++] ='�';} 
&"Ograve";	{out_buf[out_len++] ='�';} 
&"Oslash";	{out_buf[out_len++] ='�';} 
&"Otilde";	{out_buf[out_len++] ='�';} 
&"Ouml";	{out_buf[out_len++] ='�';} 
&"THORN";	{out_buf[out_len++] ='�';} 
&"Uacute";	{out_buf[out_len++] ='�';} 
&"laquo";       {out_buf[out_len++] ='�';} 
&"raquo";       {out_buf[out_len++] ='�';} 
&"Ucirc";	{out_buf[out_len++] ='�';} 
&"Ugrave";	{out_buf[out_len++] ='�';} 
&"Uuml";	{out_buf[out_len++] ='�';} 
&"Yacute";	{out_buf[out_len++] ='�';} 
&"aacute";	{out_buf[out_len++] ='�';} 
&"acirc";	{out_buf[out_len++] ='�';} 
&"aelig";	{out_buf[out_len++] ='�';} 
&"agrave";	{out_buf[out_len++] ='�';} 
&"aring";	{out_buf[out_len++] ='�';} 
&"atilde";	{out_buf[out_len++] ='�';} 
&"auml";	{out_buf[out_len++] ='�';} 
&"ccedil";	{out_buf[out_len++] ='�';} 
&"eacute";	{out_buf[out_len++] ='�';} 
&"ecirc";	{out_buf[out_len++] ='�';} 
&"egrave";	{out_buf[out_len++] ='�';} 
&"eth";	        {out_buf[out_len++] ='�';} 
&"euml";	{out_buf[out_len++] ='�';} 
&"iacute";	{out_buf[out_len++] ='�';} 
&"icirc";	{out_buf[out_len++] ='�';} 
&"igrave";	{out_buf[out_len++] ='�';} 
&"iuml";	{out_buf[out_len++] ='�';} 
&"ntilde";	{out_buf[out_len++] ='�';} 
&"oacute";	{out_buf[out_len++] ='�';} 
&"ocirc";	{out_buf[out_len++] ='�';} 
&"ograve";	{out_buf[out_len++] ='�';} 
&"oslash";	{out_buf[out_len++] ='�';} 
&"otilde";	{out_buf[out_len++] ='�';} 
&"ouml";	{out_buf[out_len++] ='�';} 
&"szlig";	{out_buf[out_len++] ='�';} 
&"thorn";	{out_buf[out_len++] ='�';} 
&"uacute";	{out_buf[out_len++] ='�';} 
&"ucirc";	{out_buf[out_len++] ='�';} 
&"ugrave";	{out_buf[out_len++] ='�';} 
&"uuml";	{out_buf[out_len++] ='�';} 
&"yacute";	{out_buf[out_len++] ='�';} 
&"yuml";	{out_buf[out_len++] ='�';} 

 /* Windows ^M */
\x0D       |
 /* Unknown entities */
&#x?{HEX}; {out_buf[out_len++] = ' ';}

 /* Default rule */
(.|\n)     {out_buf[out_len++] = yytext[0]; }

<<EOF>>   { yy_delete_buffer(YY_CURRENT_BUFFER); yyterminate(); }

%%


