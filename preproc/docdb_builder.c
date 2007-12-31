/*
 *   preproc/docdb_builder.c
 *   builds the document database
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

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include <zlib.h>

#include <pparam.h>
#include <ixemes.h>
#include <pcode.h>
#include <finnuio.h>
#include <docdb.h>
#include <hhash.h>
#include <uurl.h>

#include "rawstream.h"
#include "docdb_builder.h"

/* this should be found from stdio.h */
extern FILE *open_memstream (char **__restrict __bufloc,
                             size_t *__restrict __sizeloc) __THROW;

#define WRITE_BUF  1048576 /* 1MB */ 
#define DOCDB_COMPRESS 1

struct docdb_s{
        FILE  *toc_f;
        FILE  *data_f;
        int   final_fd;
        u32   doc_no;
};

static struct docdb_s docdb;
static uint docdb_do_compress;

void docdb_init()
{
        struct docdb_header_s tmp;
        
        char *toc_name   = NULL;
        char *data_name  = NULL;
        char *final_name = NULL;
       
        int toc_fd;
        int data_fd;
        
        PPARM_INT(docdb_do_compress, DOCDB_COMPRESS);

        final_name = pparm_common_name("docdb");
        
        asprintf(&toc_name, "%s.toc.sect", final_name);
        asprintf(&data_name, "%s.data.sect", final_name);
        
        if ((toc_fd = open(toc_name, O_CREAT | O_TRUNC | O_RDWR |
                                O_LARGEFILE, S_IREAD | S_IWRITE)) == -1)
                dub_sysdie("Couldn't open docdb toc file %s", toc_name);
        
        if ((data_fd = open(data_name, O_CREAT | O_TRUNC |
                            O_RDWR | O_LARGEFILE, S_IREAD | S_IWRITE)) == -1)
                dub_sysdie("Couldn't open data file %s", data_name);
        
        if ((docdb.final_fd = open(final_name, O_CREAT | O_TRUNC |
                            O_RDWR | O_LARGEFILE, S_IREAD | S_IWRITE)) == -1)
                dub_sysdie("Couldn't open docdb file %s", final_name);
        
        docdb.toc_f  = fdopen(toc_fd, "w");
        docdb.data_f = fdopen(data_fd, "w");
       
        if (!(docdb.toc_f && docdb.data_f))
                dub_sysdie("Couldn't initialize write buffers");
                
        if (setvbuf(docdb.data_f, NULL, _IOFBF, WRITE_BUF))
                dub_sysdie("Couldn't open data buffer");
        
        if (setvbuf(docdb.toc_f, NULL, _IOFBF, WRITE_BUF))
                dub_sysdie("Couldn't open index buffer");

        /* reserve space for the header */
        fwrite(&tmp, sizeof(struct docdb_header_s), 1, docdb.toc_f);

        free(toc_name);
        free(data_name);
        free(final_name);
}

void docdb_insert(struct obstack *tok_pos, u32 key, char *head, 
                  const char *doc_buf)
{
        struct docdb_toc_e doctoc;
        
        char  zero   = 0;
        char  *url   = head_get_field(head, "URL");
        char  *title = head_get_field(head, "TITLE");
        char  *date  = head_get_field(head, "DATE");
       
        uint  len    = obstack_object_size(tok_pos) / sizeof(gen_t);
        gen_t *pos   = (gen_t*)obstack_finish(tok_pos);
        char  *buf   = xmalloc(ENCODE_BUFFER_SIZE(len));
        u32   offs   = 0;
        u32   bytes  = 0;

        char  *membuf = NULL;
        uint  memsize = 0;
        FILE  *mem_f  = open_memstream(&membuf, &memsize);
                
        unsigned long wlen = strlen(doc_buf);
        
        if (!url)
                url = &zero;
        if (!title)
                title = &zero;
        if (!date)
                date = &zero;
       
        if (len){
                memset(buf, 0, ENCODE_BUFFER_SIZE(len)); 
                bytes = pc_encode(buf, &offs, pos, len);
                if (bytes & 7)
                        bytes = (bytes >> 3) + 1;
                else
                        bytes >>= 3;
        }
       
        /* write toc entry */
        /* this ensures that possible padding in the entry is
         * always zero. */
        memset(&doctoc, 0, sizeof(struct docdb_toc_e));
        
        doctoc.key       = key;
        doctoc.offs      = ftello64(docdb.data_f);
        doctoc.site_id   = url_site_hash(url);
        doctoc.body_hash = hash(doc_buf, wlen, 25);

        fwrite(&doctoc, sizeof(struct docdb_toc_e), 1, docdb.toc_f);
        
        /* write the document body */
        fwrite(url, strlen(url) + 1, 1, mem_f);
        fwrite(title, strlen(title) + 1, 1, mem_f);
        fwrite(date, strlen(date) + 1, 1, mem_f);
        fwrite(&bytes, 4, 1, mem_f);

        if (len){
                char          *wbuf = (char*)doc_buf;
                u32           tmp   = (u32)wlen;
                
                fwrite(buf, bytes, 1, mem_f);
                fwrite(&tmp, 4, 1, mem_f);
               
                if (docdb_do_compress){
                        int           ret  = 0;
                        unsigned long nlen = wlen * 1.2 + 12;
                        wbuf = xmalloc(nlen);
                        if ((ret = compress(wbuf, &nlen, doc_buf, wlen)) != Z_OK)
                                dub_die("Couldn't compress document. "
                                                "Zlib-err: %d", ret);
                        wlen = nlen;
                }
               
                /* this should be safe since document length is
                 * constrained elsewhere (always under < 4GB!) */
                fwrite(wbuf, wlen, 1, mem_f);

                if (docdb_do_compress)
                        free(wbuf);
        }
      
        /* write data to disk, prefixed with its length */
        fclose(mem_f);
        fwrite(&memsize, sizeof(uint), 1, docdb.data_f);
        fwrite(membuf, memsize, 1, docdb.data_f);
        
        ++docdb.doc_no;
        
        obstack_free(tok_pos, pos);
        free(buf);
        free(membuf);

        if (title != &zero)
                free(title);
        if (url != &zero)
                free(url);
        if (date != &zero)
                free(date);
}

static void sort_toc(u64 sze)
{
        void *p = mmap64(0, sze, PROT_READ | PROT_WRITE, MAP_SHARED, 
                                 fileno(docdb.toc_f), 0);
        if (p == MAP_FAILED)
                dub_sysdie("Couldn't mmap docdb toc for sorting");

        p += sizeof(struct docdb_header_s);
        qsort(p, docdb.doc_no, sizeof(struct docdb_toc_e), toc_cmp);
      
        munmap(p, sze);
}

static void glue_files()
{
        struct stat64 f_nfo;
        u64           toc_sze = 0;
        u64           dat_sze = 0;         
        
        struct docdb_header_s header = {.nof_dox    = docdb.doc_no,
                                        .compressed = docdb_do_compress};

        /* update the header */
        if (fseek(docdb.toc_f, 0, SEEK_SET) < 0)
                dub_sysdie("Couldn't seek docdb index");
        
        fwrite(&header, sizeof(struct docdb_header_s), 1, docdb.toc_f);
        
        fflush(docdb.toc_f);
        fflush(docdb.data_f);

        if (fstat64(fileno(docdb.toc_f), &f_nfo))
                dub_sysdie("Couldn't stat docdb index");
        toc_sze = f_nfo.st_size;
      
        /* the TOC must be ordered by keys to allow document
         * retrieval given a key using binary search. */
        sort_toc(toc_sze);
        
        if (fstat64(fileno(docdb.data_f), &f_nfo))
                dub_sysdie("Couldn't stat docdb data");
        dat_sze = f_nfo.st_size;

        dub_dbg("DOCDB toc %llu dat %llu", toc_sze, dat_sze);
        
        if (fio_truncate(docdb.final_fd, toc_sze + dat_sze))
                dub_sysdie("Couldn't truncate docdb to %llu bytes",
                                toc_sze + dat_sze);
        
        fio_copy_file(docdb.final_fd, 0, fileno(docdb.toc_f), toc_sze);
        fio_copy_file(docdb.final_fd, toc_sze, fileno(docdb.data_f), dat_sze);

        close(docdb.final_fd);
        fclose(docdb.data_f);
        fclose(docdb.toc_f);
}

void docdb_finish()
{
        glue_files();
}
