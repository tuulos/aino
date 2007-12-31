/*
 *   lib/qexpansion.c
 *   Query expansions
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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include <Judy.h>
#include "dub.h"
#include "mmisc.h"
#include "ixemes.h"
#include "judyaid.h"
#include "finnuio.h"
#include "qexpansion.h"

/* Note the resemblance with ixicon.c */
/* The only remarkable difference is that here we have
 * constant size keys and variable length values and
 * the ixicon has the opposite. */

struct idx_entry{
        u32 xid;
        u32 ptr;
} __attribute__((packed));

static const struct idx_entry *qexp_idx;
static const void             *qexp_body;
static uint                   ent_cnt;

static void                   *qexp_mmap;
static uint                   mmap_size;

uint qexp_loaded;

static u64 qexp_mem(const Pvoid_t qexp, u32 *cnt)
{
        u64     mem   = 0;
        Word_t  xid   = 0;
        Word_t  *qlst = NULL;
        
        JLF(qlst, qexp, xid);

        while (qlst != NULL){
                mem += (((glist*)*qlst)->len + 1) * 4;
                ++*cnt;
                JLN(qlst, qexp, xid);
        }
        
        return mem;
}

/* no expansions to token ixemes are allowed */
#if 0
static inline ixid_t check_qlst(const glist *lst)
{
        uint i = lst->len;
        while (i--)
                if (lst->lst[i] <= 0)
                        return lst->lst[i];
        return 0;
}
#endif

void create_qexp(const char *fname, const Pvoid_t qexp)
{
        int   fd;
        u32   cnt       = 0;
        u64   sze       = qexp_mem(qexp, &cnt);
        u32   ptr       = 0;
        
        void  *mm_idx   = NULL;
        void  *mm_body  = NULL;
        void  *mm_start = NULL;
        
        Word_t xid      = 0;
        Word_t *qlst    = NULL;
        
        if ((fd = open(fname, O_CREAT | O_TRUNC | O_RDWR,
                              S_IREAD | S_IWRITE)) == -1)
                dub_sysdie("Couldn't open qexp file %s", fname);

        /* add the index size */
        /* cnt * (xid, pos) + cnt */
        sze += cnt * sizeof(struct idx_entry) + 4;
        
        if (sze > INT32_MAX) 
                dub_die("Qexp file would take %llu but only "
                        "32bit offsets are supported.", sze);
        
        if (fio_truncate(fd, PAGE_ALIGN(sze)))
                dub_sysdie("Couldn't qexp file to %llu bytes.", sze);
 
        mm_idx = mm_start = mmap(0, PAGE_ALIGN(sze), PROT_WRITE, 
                                MAP_SHARED, fd, 0);
        
        if (mm_start == MAP_FAILED)
                dub_sysdie("Couldn't mmap qexp file %s", fname);

        mm_body = mm_start + (cnt * sizeof(struct idx_entry)) + 4;

        memcpy(mm_idx, &cnt, 4);
        mm_idx += 4;

        JLF(qlst, qexp, xid);

        while (qlst != NULL){

                uint             tmp = 0;
                struct idx_entry ent = {xid, ptr};
        
                /* expansion list */
                tmp = (((glist*)*qlst)->len + 1) * 4;
                memcpy(mm_body, (glist*)*qlst, tmp);
                mm_body += tmp;

                /* the corresponding index entry */
                memcpy(mm_idx, &ent, sizeof(struct idx_entry));
                mm_idx += sizeof(struct idx_entry);

                ptr += tmp;
                JLN(qlst, qexp, xid);
        }

        if (munmap(mm_start, sze))
                dub_sysdie("Couldn't munmap qexp. Disk full?");
                
        close(fd);
}

void close_qexp()
{
        munmap(qexp_mmap, mmap_size);
        qexp_loaded = 0;
}

uint load_qexp(const char *fname, uint ignore_notexist)
{
        struct stat     f_nfo;
        int             fd = 0;
        
        if ((fd = open(fname, O_RDONLY)) == -1){
                if (!ignore_notexist)
                        dub_sysdie("Couldn't open qexp file %s", fname);
                else
                        return 1;
        }

        if (fstat(fd, &f_nfo))
                dub_sysdie("Couldn't stat qexp file %s", fname);
        
        mmap_size = f_nfo.st_size;
        qexp_mmap = mmap(0, mmap_size, PROT_READ, MAP_SHARED, fd, 0);
        
        if (qexp_mmap == MAP_FAILED)
                dub_sysdie("Couldn't mmap qexp file %s", fname);

        ent_cnt   = *(u32*)qexp_mmap;
        dub_dbg("Ent cnt %u", ent_cnt);

        qexp_idx  = (struct idx_entry*)(qexp_mmap + 4);
        qexp_body = qexp_idx + ent_cnt;

        close(fd);
        
        qexp_loaded = 1;
        return 0;
}

Pvoid_t judify_qexp()
{
        uint i;
        Pvoid_t ret = NULL;
       
        for (i = 0; i < ent_cnt; i++){

                Word_t xid = (Word_t)qexp_idx[i].xid;
                Word_t *ptr;
                
                JLI(ptr, ret, xid);
                *ptr = (Word_t)(qexp_body + qexp_idx[i].ptr);
        }       

        return ret;
}

Pvoid_t qexp_merge(const Pvoid_t nexp)
{
        Pvoid_t oexp  = judify_qexp();
        Pvoid_t new   = NULL;
        Word_t  idx   = 0;
        Word_t  *val  = NULL;
        Word_t  *nval = NULL;
        uint    tmp   = 0;
        
        JLF(val, oexp, idx);

        while (val != NULL){
                
                glist *merged = NULL;
                glist *nlst   = NULL;
                glist *olst   = NULL;
                
                JLG(nval, nexp, idx);
                if (!nval){
                        merged = (glist*)*val;
                        goto next;
                }
                
                nlst = (glist*)*nval;
                olst = (glist*)*val;
               
                merged = merge_glists(nlst, olst);
next:
                JLI(nval, new, idx);
                *nval = (Word_t)merged;

                JLN(val, oexp, idx);
        }
        
        /* go through previously unseen indices of the new array */
        idx = 0;
        JLF(val, nexp, idx);

        while (val != NULL){
                Word_t *tst = NULL;
                        
                JLG(tst, oexp, idx);
                if (!tst){
                        JLI(nval, new, idx);
                        *nval = *val;
                }
                
                JLN(val, nexp, idx);
        }

        JLFA(tmp, oexp);

        return new;
}

static int idx_cmp(const void *e1, const void *e2)
{
        const struct idx_entry *i1 = (const struct idx_entry*)e1;
        const struct idx_entry *i2 = (const struct idx_entry*)e2;

        if (i1->xid > i2->xid)
                return 1;
        else if(i1->xid < i2->xid)
                return -1;
        return 0;
}

const glist *get_expansion(u32 xid)
{
        void *p = bsearch(&xid, qexp_idx, ent_cnt,
                                sizeof(struct idx_entry), idx_cmp);
        if (!p)
                return NULL;

        return qexp_body + ((struct idx_entry*)p)->ptr;
}

glist *expand_all(const glist *lst)
{
        Pvoid_t set  = NULL;
        glist   *ret = NULL;
        uint i, j, tst;
        Word_t idx;

        for (i = 0; i < lst->len; i++){
                
                const glist *exp = get_expansion(lst->lst[i]);
                
                J1S(tst, set, lst->lst[i]);
                
                if (!exp)
                        continue;

                for (j = 0; j < exp->len; j++){
                        J1S(tst, set, exp->lst[j]);
                }
        }

        J1C(i, set, 0, -1);

        ret = xmalloc(i * 4 + sizeof(glist));
        ret->len = i;

        idx = i = 0;
        J1F(tst, set, idx);
        while (tst){
                ret->lst[i++] = idx;
                J1N(tst, set, idx);
        }
        
        J1FA(tst, set);
        return ret;
}
