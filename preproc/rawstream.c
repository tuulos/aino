/*
 *   preproc/rawstream.c
 *   Handles untokenized document stream
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

#define _GNU_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include <Judy.h>

#include "rawstream.h"
#include <dub.h>
#include <mmisc.h>

/* head operations */

char *head_get_field(const char *head, const char* field)
{
        const char *pos  = NULL;
        const char *end  = NULL;
        char *ret        = NULL;
      
        /* match start of the header or start of any line
         * in the header */
        if (!strncmp(head, field, strlen(field))){
                pos = head;
                pos += strlen(field) + 1;
        }else{
                char *tmp = NULL;
                asprintf(&tmp, "\n%s", field);
                pos = strstr(head, tmp);
                if (pos)
                        pos += strlen(field) + 2;
                free(tmp);
        }
        
        if (!pos)
                return NULL;
       
        end = strchr(pos, '\n'); 
        if (!end)
                end = pos + strlen(pos);

        ret = xmalloc((end - pos) + 1);
        memcpy(ret, pos, end - pos);
        ret[end - pos] = 0;
       
//        dub_dbg("FIELD <%s> VALUE <%s>", field, ret);
        
        return ret;
}

char* head_parse(const char **doc_buf)
{
        char *head      = NULL;
        const char *end = strstr(*doc_buf, "\n---\n");
        
        if (!end)
                dub_die("Missing document header");
        else{
                head = xmalloc((end - *doc_buf) + 1);
                memcpy(head, *doc_buf, end - *doc_buf);
                head[end - *doc_buf] = 0;
        }

        *doc_buf = end + 5;

        return head;
}

uint head_meta_match(const char *head, u32 first, u32 last)
{
        char *meta = head_get_field(head, "META");
        char *tok  = meta;
        int match  = 0;
        int val    = 0;
        
        if (!meta)
                return 0;
       	
	errno = 0;
	
        while ((val = strtoul(tok, &tok, 10))){

               if (val <= 0 || errno)
                       dub_sysdie("Invalid META field \"%s\"", meta);
               
               if (val > last)
                       break;
               
               if (val >= first){
                       match = val;
                       break;
               }
        }
        
        free(meta);
        return match;
}

Pvoid_t head_meta_bag(const char *head)
{
        char *tok  = NULL;
        char *meta = tok = head_get_field(head, "META");
        u32  val   = 0;
        
        Pvoid_t meta_bag = NULL;
        
        if (!meta)
                return meta_bag;
        
        while ((val = (u32)strtoul(tok, &tok, 10))){
                
                int tst;
                
                if (!val)
                        dub_die("Invalid meta field \"%s\"", meta);
               
                J1S(tst, meta_bag, val);    
        }
        free(meta);

        return meta_bag;
}

char *head_meta_add(char *head, u32 val)
{
        char       empty = 0;
        char       *ret  = NULL;
        char       *pos  = NULL;
        char       *meta = NULL;
        char       *tok  = NULL;
        char       *prev = NULL;
        char       *end  = NULL;
        int        cur   = 0;
        
        /* match start of the header or start of any line
         * in the header */
        if (!strncmp(head, "META", 4))
                pos = head;
        else{
                pos = strstr(head, "\nMETA");
                if (pos) ++pos;
        }
        
        /* no previous META */
        if (!pos){
                asprintf(&ret, "META %u\n%s", val, head);
                free(head);
                return ret;
        }
        
        if (pos != head){
                char *p = pos - 1;
                *p = 0;
        }else
                head = &empty;
        
        end = strchr(pos, '\n'); 
        if (!end)
                end = &empty;
        
        meta = tok = prev = pos + 5;
       	errno = 0;

        while ((cur = strtoul(tok, &tok, 10))){

               if (cur <= 0 || errno)
                       dub_sysdie("Invalid META string \"%s\"", meta);
               
               if (cur > val)
                       break;
               else if (cur == val){
                       /* no changes if the value exists already */
                       asprintf(&ret, "%s\nMETA %s", head, meta);
                       if (head != &empty) free(head);
                       return ret;
               }
              
               prev = tok;
        }
        
        if (prev != meta){
                *prev = 0;
                ++prev; 
        }
       
        if (prev == meta)
                asprintf(&ret, "%s\nMETA %u %s", head, val, meta);
        else{
                if (cur > val)
                        asprintf(&ret, "%s\nMETA %s %u %s",
                                        head, meta, val, prev);
                else
                        asprintf(&ret, "%s\nMETA %s %u\n%s",
                                        head, meta, val, prev);
        }

        if (head != &empty)
                free(head);
        
        return ret;
}

/* body operations */

struct rawdoc *pull_document()
{
        static char lenbuf[20];
        static char *docbuf;
        static uint docbuf_sze;
        static struct rawdoc doc;
        
        uint doclen = 0;

        /* skip whitespace between documents */
        do{
                if (!fgets(lenbuf, 20, stdin))
                        return NULL;
        }while(isspace(lenbuf[0]));
        
        if (!(doclen = strtoul(lenbuf, NULL, 10)))
                dub_die("Invalid content length field");

        if (doclen + 1 > docbuf_sze){
                
                docbuf_sze += doclen + 1;
                if (docbuf)
                        docbuf = realloc(docbuf, docbuf_sze);
                else
                        docbuf = malloc(docbuf_sze);
                        
                if (!docbuf)
                        dub_sysdie("Couldn't grow doc buffer to %u bytes",
                                        docbuf_sze);
        }

        if (fread(docbuf, doclen, 1, stdin) < 1)
                dub_die("Got EOF in the middle of a document");
        
        docbuf[doclen] = 0;

        doc.body = docbuf;
        doc.head = head_parse(&doc.body);
        doc.body_size = doclen - (strlen(doc.head) + 5);

        /*
        dub_dbg("HEAD <%s>", doc.head);
        dub_dbg("DOC <%s>", doc.body);
        */
        
        return &doc;
}
                
void push_document(struct rawdoc *doc)
{
        uint hlen = strlen(doc->head);

        printf("%u\n%s\n---\n", hlen + doc->body_size + 5, doc->head);

        if (doc->body)
                fwrite(doc->body, doc->body_size, 1, stdout);

        free(doc->head);
}

#if 0

int main(int argc, char **argv)
{
        struct rawdoc *doc = NULL;

        dub_init();
        
        while ((doc = pull_document())){

                dub_dbg("Doc (%u) <%s>\n", doc->body_size, doc->body);
                dub_dbg("hgead <%s>", doc->head);
                
                push_document(doc);
        }
}

#endif
