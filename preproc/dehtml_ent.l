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
&"copy";   	{out_buf[out_len++] ='©';} 
&"AElig";	{out_buf[out_len++] ='Æ';} 
&"Aacute";	{out_buf[out_len++] ='Á';} 
&"Acirc";	{out_buf[out_len++] ='Â';} 
&"Agrave";	{out_buf[out_len++] ='À';} 
&"Aring";	{out_buf[out_len++] ='Å';} 
&"Atilde";	{out_buf[out_len++] ='Ã';} 
&"Auml";	{out_buf[out_len++] ='Ä';} 
&"Ccedil";	{out_buf[out_len++] ='Ç';} 
&"ETH";	        {out_buf[out_len++] ='Ð';} 
&"Eacute";	{out_buf[out_len++] ='É';}  
&"Ecirc";	{out_buf[out_len++] ='Ê';} 
&"Egrave";	{out_buf[out_len++] ='È';} 
&"Euml";	{out_buf[out_len++] ='Ë';} 
&"Iacute";	{out_buf[out_len++] ='Í';} 
&"Icirc";	{out_buf[out_len++] ='Î';} 
&"Igrave";	{out_buf[out_len++] ='Ì';} 
&"Iuml";	{out_buf[out_len++] ='Ï';} 
&"Ntilde";	{out_buf[out_len++] ='Ñ';} 
&"Oacute";	{out_buf[out_len++] ='Ó';} 
&"Ocirc";	{out_buf[out_len++] ='Ô';} 
&"Ograve";	{out_buf[out_len++] ='Ò';} 
&"Oslash";	{out_buf[out_len++] ='Ø';} 
&"Otilde";	{out_buf[out_len++] ='Õ';} 
&"Ouml";	{out_buf[out_len++] ='Ö';} 
&"THORN";	{out_buf[out_len++] ='Þ';} 
&"Uacute";	{out_buf[out_len++] ='Ú';} 
&"laquo";       {out_buf[out_len++] ='«';} 
&"raquo";       {out_buf[out_len++] ='»';} 
&"Ucirc";	{out_buf[out_len++] ='Û';} 
&"Ugrave";	{out_buf[out_len++] ='Ù';} 
&"Uuml";	{out_buf[out_len++] ='Ü';} 
&"Yacute";	{out_buf[out_len++] ='Ý';} 
&"aacute";	{out_buf[out_len++] ='á';} 
&"acirc";	{out_buf[out_len++] ='â';} 
&"aelig";	{out_buf[out_len++] ='æ';} 
&"agrave";	{out_buf[out_len++] ='à';} 
&"aring";	{out_buf[out_len++] ='å';} 
&"atilde";	{out_buf[out_len++] ='ã';} 
&"auml";	{out_buf[out_len++] ='ä';} 
&"ccedil";	{out_buf[out_len++] ='ç';} 
&"eacute";	{out_buf[out_len++] ='é';} 
&"ecirc";	{out_buf[out_len++] ='ê';} 
&"egrave";	{out_buf[out_len++] ='è';} 
&"eth";	        {out_buf[out_len++] ='ð';} 
&"euml";	{out_buf[out_len++] ='ë';} 
&"iacute";	{out_buf[out_len++] ='í';} 
&"icirc";	{out_buf[out_len++] ='î';} 
&"igrave";	{out_buf[out_len++] ='ì';} 
&"iuml";	{out_buf[out_len++] ='ï';} 
&"ntilde";	{out_buf[out_len++] ='ñ';} 
&"oacute";	{out_buf[out_len++] ='ó';} 
&"ocirc";	{out_buf[out_len++] ='ô';} 
&"ograve";	{out_buf[out_len++] ='ò';} 
&"oslash";	{out_buf[out_len++] ='ø';} 
&"otilde";	{out_buf[out_len++] ='õ';} 
&"ouml";	{out_buf[out_len++] ='ö';} 
&"szlig";	{out_buf[out_len++] ='ß';} 
&"thorn";	{out_buf[out_len++] ='þ';} 
&"uacute";	{out_buf[out_len++] ='ú';} 
&"ucirc";	{out_buf[out_len++] ='û';} 
&"ugrave";	{out_buf[out_len++] ='ù';} 
&"uuml";	{out_buf[out_len++] ='ü';} 
&"yacute";	{out_buf[out_len++] ='ý';} 
&"yuml";	{out_buf[out_len++] ='ÿ';} 

 /* Windows ^M */
\x0D       |
 /* Unknown entities */
&#x?{HEX}; {out_buf[out_len++] = ' ';}

 /* Default rule */
(.|\n)     {out_buf[out_len++] = yytext[0]; }

<<EOF>>   { yy_delete_buffer(YY_CURRENT_BUFFER); yyterminate(); }

%%


