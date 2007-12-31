/*
 *   preproc/dehtml.l
 *   Flex rules for dehtmlization
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

DOCTYPE (doctype)
HEAD    (head)
TITLE   (title)
SCRIPT  (script)
STYLE   (style)
NOSCRIPT (noscript)
ENDTAG  "</"
ATTR    [[:alpha:]]+[[:space:]]*=[[:space:]]*
ATTRVAL [^[:space:]<>'\"]+
QUOTE   [']
DQUOTE  ["\""]
ALL     (.|\n)
COMMENT  "<!--"
ECOMMENT "-->"

TAG     (p|blockquote|hr|br|dir|menu|form|fieldset|legend|label|input|select|option|textarea|isindex|frame|noframes|frameset|iframe|table|caption|tr|td|tfoot|th|h[1-6]|dl|dd|body|dt|ul|ol|li|tbody|html|head|a|abbr|acronym|address|applet|area|b|base|basefont|bdo|big|button|center|cite|code|col|colgroup|del|dfn|div|em|font|i|img|ins|kbd|link|map|ismap|meta|object|param|pre|q|s|samp|small|span|strike|strong|sub|sup|target|title|tt|u|var|!doctype|\?xml|header|nextid|section|listing|xmp|plaintext|align|blink|embed|ilayer|keygen|layer|multicol|noembed|nolayer|nosave|spacer|inlineinput|bgsound|comment|marquee|xml|o:p|csaction|csactions|csactiondict|csscriptdict|csactionitem|csobj|ruby|rbc|rtc|rb|rt|rp|nobr)

%{
#include "dehtml.h"
%}

%x instyle
%x inscript
%x innoscript
%x inhead
%x intag
%x inattr
%x indquote
%x inquote
%x incomment
%%
                         yy_scan_string(doc_buf);
                         
  /* !DOCTYPE -lines, be gone */
[<][!]{DOCTYPE}.*        {}

  /* <!-- Comments -->, be gone */
{COMMENT}                { BEGIN(incomment); }
<incomment>{ECOMMENT}    { BEGIN(INITIAL); }
<incomment>{ALL}         {}

  /* The whole <head>...</head> be gone, but save the title */
[<]{HEAD}                  { BEGIN(inhead); tag_matched = 1; }
<inhead>[<]{TITLE}[>][^<]* { 
                              strncpy(title, &yytext[7], MAX_TITLE_LEN);
                              title[yyleng - 7 > MAX_TITLE_LEN ? 
                                    MAX_TITLE_LEN: yyleng - 7] = 0;         
                            }
<inhead>{ENDTAG}{HEAD}[>]  { BEGIN(INITIAL); }
<inhead>{ALL}              {}

  /* <script>...</script> be gone */
[<]{SCRIPT}                   { BEGIN(inscript); }
<inscript>{ENDTAG}{SCRIPT}[>] { BEGIN(INITIAL); }
<inscript>{ALL}               {}

  /* <style>...</style> be gone (shouldn't exist outside the head anyway) */
[<]{STYLE}                    { BEGIN(instyle); }
<instyle>{ENDTAG}{STYLE}[>]   { BEGIN(INITIAL); }
<instyle>{ALL}                {}

  /* <noscript>...</noscript> be gone */
[<]{NOSCRIPT}                     { BEGIN(innoscript); }
<innoscript>{ENDTAG}{NOSCRIPT}[>] { BEGIN(INITIAL); }
<innoscript>{ALL}                 {}

  /* 
     Tag matching magic:
     1. Match the tag beginning, go to intag mode
     2. If an attribute (something=) is found, go to inattr mode 
     3. Remove everything within attribute values. Attribute ends 
        either with a proper quote or a tag beginning. This kludge
        is needed to handle unbalanced quotes.
     4. Everything within a tag is removed 
  */

<INITIAL,inquote,indquote>[<]{TAG} { tag_matched = 1; 
                                     BEGIN(intag); 
                                   }
<intag>{ATTR}                      { BEGIN(inattr); }
<inattr>{DQUOTE}                   { BEGIN(indquote); }
<inattr>{QUOTE}                    { BEGIN(inquote); }
<inquote>([^'])                    |
<indquote>([^"])                   {}
<inquote>{QUOTE}                   |
<indquote>{DQUOTE}                 |

 /* Attribute value without quotes */
<inattr>{ATTRVAL}                  |
 /* Fallback for the previous, if the value is empty */
<inattr>(.|\n)                     { BEGIN(intag); }

<intag>[/]{0,1}>                   { out_buf[out_len++] = ' ';
                                     BEGIN(INITIAL); 
                                   }
{ENDTAG}{TAG}[>]                   |
<intag>(.|\n)                      {}

(.|\n)                             { out_buf[out_len++] = yytext[0]; }

<<EOF>>   { yy_delete_buffer(YY_CURRENT_BUFFER); yyterminate(); }

%%
  
          
