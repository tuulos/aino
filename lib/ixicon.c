/*
 *   lib/lexicon.c
 *   Maintains ixeme->xid mapping
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
 * 
 * File structure:
 * 
 * TOC:
 * 
 * ----------------------------------------------------
 * | entry_offs_1 | entry_offs_2 | ... | entry_offs_N |   u32 * ixi_cnt
 * ----------------------------------------------------
 *
 * ENTRIES:
 * 
 * ---------------------------------
 * | ixeme_str (char*) | xid (u32) |
 * ---------------------------------
 *  ...
 *  
 */


#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include <Judy.h>
#include "dub.h"
#include "mmisc.h"
#include "ixicon.h"
#include "finnuio.h"

static const u32  *ixi_idx;
static const char *ixi_body;
static uint       ixi_cnt;

static u64 ixicon_mem(const Pvoid_t ixicon, u32 *ix_cnt)
{
        /* 4 bytes for number of ixemes */
        u64         mem  = 4;
        Word_t      *xid = NULL;
        static char ix[MAX_TOK_LEN];
       
        *ix_cnt = 0;
        strcpy(ix, "");
        JSLF(xid, ixicon, ix);
   
        while(xid != NULL){
                /* 4 bytes for xid, one for zero */
                mem += strlen(ix) + 4 + 1; 
                ++(*ix_cnt);
                JSLN(xid, ixicon, ix);
        }

        return mem;
}

void create_ixicon(const char *fname, const Pvoid_t ixicon)
{
        void        *mm_idx   = NULL;
        void        *mm_body  = NULL;
        void        *mm_start = NULL;
        
        Word_t      *xid       = NULL;
        u32         ixeme_cnt  = 0;
        u64         ixicon_sze = ixicon_mem(ixicon, &ixeme_cnt);
        uint        ix_pos     = 0;
        
        static char ix[MAX_TOK_LEN];
        int         fd;
        
        if ((fd = open(fname, O_CREAT | O_TRUNC | O_RDWR,
                                        S_IREAD | S_IWRITE)) == -1)
                dub_sysdie("Couldn't open ixicon %s", fname);
        
        ixicon_sze += (ixeme_cnt + 1) * 4; /* space needed by the index */
        
        if (ixicon_sze > INT32_MAX)
                dub_die("Ixicon would take %llu but we support only "
                        "32bit offsets.", ixicon_sze);
        
        if (fio_truncate(fd, PAGE_ALIGN(ixicon_sze)))
                dub_sysdie("Couldn't truncate ixicon to %llu bytes.", 
                                ixicon_sze);
 
        mm_idx = mm_start = mmap(0, PAGE_ALIGN(ixicon_sze), PROT_WRITE, 
                                MAP_SHARED, fd, 0);
        
        if (mm_start == MAP_FAILED)
                dub_sysdie("Couldn't mmap ixicon %s", fname);

        mm_body = mm_start + (ixeme_cnt + 1) * 4 + 4;
       
        /* first write the number of ixemes */
        memcpy(mm_idx, &ixeme_cnt, 4);
        mm_idx += 4;
        
        strcpy(ix, "");
        JSLF(xid, ixicon, ix);
    
        while(xid != NULL){
                
                uint ix_len = strlen(ix) + 1;
                
                /* ixeme */
                memcpy(mm_body, ix, ix_len);
                mm_body += ix_len;
                
                /* ixeme id */
                memcpy(mm_body, (u32*)xid, 4);
                mm_body += 4;
                
                /* write the index entry */
                memcpy(mm_idx, &ix_pos, 4);
                mm_idx += 4;
                ix_pos += ix_len + 4;
                
                JSLN(xid, ixicon, ix);
        }
        
        memcpy(mm_idx, &ix_pos, 4);
        
        if (munmap(mm_start, ixicon_sze))
                dub_sysdie("Couldn't munmap ixicon. Disk full?");

        close(fd);

        dub_dbg("Ixicon created");
}

void load_ixicon(const char *fname)
{
        struct stat     f_nfo;
        int             fd = 0;
        
        if ((fd = open(fname, O_RDONLY)) == -1)
                dub_sysdie("Couldn't open ixicon %s", fname);

        if (fstat(fd, &f_nfo))
                dub_sysdie("Couldn't stat ixicon %s", fname);
        
        ixi_idx = (const u32*)mmap(0, f_nfo.st_size, PROT_READ,
                                        MAP_SHARED, fd, 0);
        if (ixi_idx == MAP_FAILED)
                dub_sysdie("Couldn't mmap ixicon %s", fname);

        ixi_cnt  = *ixi_idx;
        ixi_idx  += 1;
        ixi_body = (const char*)(ixi_idx + (ixi_cnt + 1));
        
        close(fd);
}

Pvoid_t judify_ixicon()
{
        Pvoid_t ixicon = NULL;
        uint    i;

        for (i = 0; i < ixi_cnt; i++){
                Word_t *xid = NULL;
                JSLI(xid, ixicon, &ixi_body[ixi_idx[i]]);
                *xid = *(u32*)&ixi_body[ixi_idx[i + 1] - 4];
        }

        return ixicon;
}

Pvoid_t reverse_ixicon()
{
        Pvoid_t rixicon = NULL;
        uint    i;

        for (i = 0; i < ixi_cnt; i++){
                Word_t *ix   = NULL;
                u32 xid = *(u32*)&ixi_body[ixi_idx[i + 1] - 4];

                /* skip all entries containing a dot (i.e. site names) */
                /*
                if (index(&ixi_body[ixi_idx[i]], '.'))
                        continue;
                */
                
                JLI(ix, rixicon, xid);
                *ix = (Word_t)&ixi_body[ixi_idx[i]];
        }

        return rixicon;
}

void copy_sites(Pvoid_t judy)
{
        uint i;

        for (i = 0; i < ixi_cnt; i++){

                if (index(&ixi_body[ixi_idx[i]], '.')){
                        
                        u32 *xid = NULL;
                        JSLI(xid, judy, &ixi_body[ixi_idx[i]]);
                        *xid = *(u32*)&ixi_body[ixi_idx[i + 1] - 4];
                }
        }
}

/* forward lookup */
/* binary search the matching ixeme id */
u32 get_xid(const char *ixeme)
{
        int  left  = 0;
        int  right = ixi_cnt;
        uint  idx   = 0;
        
        while (left <= right){
               
                int cmp = 0;
               
                idx = (left + right) >> 1;
                cmp = strcasecmp(&ixi_body[ixi_idx[idx]], ixeme);
               
                if (cmp == 0){
                        return *(u32*)&ixi_body[ixi_idx[idx + 1] - 4];
                }else if (cmp > 0)
                        right = idx - 1;
                else
                        left = idx + 1;
        }
        
        return 0;
}

/* reverse lookup */
/* heavy operation -- use only for debugging */
/* use reverse_ixicon() for anything real */
const char *get_ixeme(u32 xid)
{
        uint    i;

        for (i = 0; i < ixi_cnt; i++){
                u32 txid = *(u32*)&ixi_body[ixi_idx[i + 1] - 4];
                
                if (index(&ixi_body[ixi_idx[i]], '.'))
                        continue;
                
                if (txid == xid)
                        return &ixi_body[ixi_idx[i]];
        }
  
        return NULL;
}

#ifdef IXICON_MAIN

int main(int argc, char **argv){
        
        dub_init();
        
        dub_dbg("IXICON! >> %s", argv[1]);
        
        load_ixicon(argv[1]);

        dub_dbg("IX <%s> XID <%u>", argv[2], get_xid(argv[2]));
}

#endif
