/*
 *   preproc/normality_prior.c
 *   
 *   Copyright (C) 2007-2008 Ville H. Tuulos
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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <Judy.h>

#include <dub.h>
#include <pparam.h>
#include <gglist.h>
#include <rrice.h>
#include <dexvm.h>
#include <mmisc.h>
#include <finnuio.h>

#include "ixeme_stats.h"

#define TOPWORDS 10000
#define WINDOW_LENGTH 10
#define STATS 1

static uint segment_size;

doc_e *load_info_sect(const char *file)
{
        struct stat64  f_nfo;
        int          fd = 0;
        void         *p = NULL;  
        
        if ((fd = open(file, O_RDWR | O_LARGEFILE)) == -1)
                dub_sysdie("Couldn't open file %s", file);

        if (fstat64(fd, &f_nfo))
                dub_sysdie("Couldn't stat file %s", file);
        
        uint size = PAGE_ALIGN(f_nfo.st_size);
        
        p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (p == MAP_FAILED)
                dub_sysdie("Couldn't mmap stats file %s", file);

        return (doc_e*)p;
}

Pvoid_t find_topwords(int topk)
{
        Pvoid_t top = NULL;
        Pvoid_t lang_freqs = NULL;

        int i, tst;
        
        for (i = 0; i < istats_len; i++){
                Word_t *ptr;
                JLI(ptr, lang_freqs, istats[i].lang_code);
                if (*ptr < topk){
                        J1S(tst, top, istats[i].xid);
                        ++*ptr;
                }
        }
        return top;
}

float estimate_normality(u32 sid, u32 last_sid, 
        const Pvoid_t topwords, uint winlen)
{
        u32 win, tst, i;
        u32 num_win = 0;
        u32 num_ok = 0;

        for (;sid < last_sid; sid++){
                tst = win = 0;
                const u32 *seg = decode_segment(sid, segment_size);
                if (!seg)
                        continue;

                for (i = 0; i < segment_size && seg[i]; i++){
                        if (i >= winlen){
                                if (win)
                                        ++num_ok;
                                ++num_win;
                        }
                        win >>= 1;
                        J1T(tst, topwords, seg[i]);
                        win |= tst << winlen;
                }
        }
        
        if (num_win)
                return num_ok / (float)num_win;
        else
                return 0.0;
}

int main(int argc, char **argv)
{
        char    *basename = NULL;
        char    *dname    = NULL;

        static uint stats[11];
        uint did, iblock = 0;
        uint topk = 0;
        uint winlen = 0;
        uint print_stats = 0;

        dub_init();
        
        PPARM_INT(topk, TOPWORDS);
        PPARM_INT(print_stats, STATS);
        PPARM_INT(winlen, WINDOW_LENGTH);
        PPARM_INT_D(iblock, IBLOCK);
        
        PPARM_INT_D(segment_size, SEGMENT_SIZE);
       
        if (winlen > 31)
                dub_die("WINDOW_LENGTH must be <= 31");

        open_section(POS, iblock);
        open_section(INFO, iblock);

        dub_msg("DIS %u", DEX_NOF_DOX);

        const char *stats_name = 
                stats_name = pparm_common_name("istats");
        load_istats(stats_name);
        
        basename = pparm_common_name("info");
        asprintf(&dname, "%s.%u.sect", basename, iblock);
        doc_e *nfo = load_info_sect(dname);
        free(dname);
        free(basename);

        basename = pparm_common_name("fwlayers");
        open_section_i(basename, FW, iblock);
        asprintf(&dname, "%s.%u.nfo", basename, iblock);
        int fd;
        if ((fd = open(dname, O_RDONLY, 0)) == -1)
                dub_sysdie("Couldn't open file %s", dname);
        fio_read(fd, &dex_header.fw_layers, sizeof(dex_header.fw_layers));
        close(fd);

        dub_msg("DKD %u", dex_header.fw_layers[1].idx);

        Pvoid_t topwords = find_topwords(topk);

        for (did = 0; did < DEX_NOF_DOX; did++){
                float p;
                if (did == DEX_NOF_DOX - 1)
                        p = estimate_normality(nfo[did].first_sid + 1,
                                DEX_NOF_SEG, topwords, winlen);
                else
                        p = estimate_normality(nfo[did].first_sid + 1,
                                nfo[did + 1].first_sid, topwords, winlen);
                
                nfo[did].prior = p;
                ++stats[(int)(nfo[did].prior * 10)];
        }
        if (print_stats){
                printf("Distribution of document priors:\n");
                for (did = 0; did < 11; did++)
                        printf("%.1f %u\n", did * 0.1, stats[did]);
        }

        return 0;
}
