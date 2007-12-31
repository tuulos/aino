/*
 *   preproc/dehtml.c
 *   Dethtmlificator
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

#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>

#include <dub.h>
#include <pparam.h>
#include <rawstream.h>

#include "dehtml.h"

/* verify htmlness, i.e. document contains at least one valid tag */
#define CHECK_HTML 0

/* do not skip non-html documents, just prune them to nothingness */
#define KEEP_ALL   1

int dehlex();
int entlex();
int whilex();

const char *doc_buf;
char       *out_buf;
uint       out_len;
uint       tag_matched;

static uint max_len;

char title[MAX_TITLE_LEN + 1];

static void expand_entities(char *buf)
{
        const char *p  = buf;
        uint       c   = 0;
        uint       len = strlen(buf);
        
        /* count angles */
        while (*p){
                if (*p == '<' || *p == '>') ++c;
                p++;
        }
        
        doc_buf = buf;
        
        if (c * 3 + len > max_len){
                if (out_buf) free(out_buf);
                out_buf = xmalloc(len + c * 3 + 1);
                max_len = len + c * 3 + 1;
        }

        out_len = 0;
        entlex();
        out_buf[out_len] = 0;
}

int main(int argc, char **argv)
{
        struct rawdoc *doc;
        
        char       *aux       = NULL;
        uint       aux_len    = 0;
        uint       keep_all   = 0;
        uint       check_html = 0;
        
        dub_init();
        
        out_buf = NULL;
        out_len = 0;
        
        PPARM_INT(keep_all, KEEP_ALL);
        PPARM_INT(check_html, CHECK_HTML);
        
        while ((doc = pull_document())){

                char *tmp;
                
                /*
        while (next_document(STDIN_FILENO, &stream)){
                
                char *head  = head_parse(&stream);
                uint len    = strlen(stream);
                */
                if (doc->body_size > max_len){
                        max_len = doc->body_size + 1;
                        if (out_buf)
                                free(out_buf);
                        out_buf = xmalloc(max_len);
                }
               
                /* de-html */
                *title      = 0;
                out_len     = 0;
                tag_matched = 0;
                doc_buf     = doc->body;

                dehlex();

                if (check_html && !tag_matched){
                        if (keep_all){
                                doc->body_size = 0;
                                doc->body = NULL;
                                push_document(doc);
                                //goto next;
                                continue;
                        }else
                                continue;
                }
                                
                out_buf[out_len] = 0;
                
                if (out_len > aux_len){
                        if (aux)
                                free(aux);
                        aux = xmalloc(out_len + 1);
                        aux_len = out_len;
                }
                strcpy(aux, out_buf);
              
                /* trim title */
                if (title[0]){
                        
                        char *p;
                        expand_entities(title);
                        p = out_buf;
                        
                        while (*p){
                                if (*p == '\n') *p = ' ';
                                ++p;
                        }
                
                        asprintf(&tmp, "%s\nTITLE %s\n", doc->head, out_buf);
                        free(doc->head);
                        doc->head = tmp;
                }
               
                /* expand entities in the document body */
                expand_entities(aux);

                if (out_len > aux_len){
                        if (aux)
                                free(aux);
                        aux = xmalloc(out_len + 1);
                        aux_len = out_len;
                }
                strcpy(aux, out_buf);
                
                /* normalize whitespace */
                out_len = 0;
                doc_buf = aux;
                
                whilex();

                doc->body_size = out_len;
                doc->body = out_buf;
                
                push_document(doc);
//next:
//                fflush(stdout);
//                free(head); 
        }

        return 0;
}
