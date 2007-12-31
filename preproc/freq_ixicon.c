/*
 *   preproc/freq_ixicon.c
 *   Re-construct the ixicon based on ixeme occurrences
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


#include <stdlib.h>
#include <stdio.h>
#include <asm/types.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <Judy.h>

#include <dub.h>
#include <ixicon.h>
#include <ixemes.h>
#include <pparam.h>

#include "ixeme_stats.h"

#define NOF_FREQUENT 100

static int fix_cmp(const void *i1, const void *i2)
{
        struct istat_entry *f1 = (struct istat_entry*)i1;
        struct istat_entry *f2 = (struct istat_entry*)i2;

        if (f1->freq > f2->freq)
                return -1;
        else if(f1->freq < f2->freq)
                return 1;
        return 0;
}


int main(int argc, char **argv)
{
        Pvoid_t rixicon   = NULL;
        Pvoid_t new_ixi   = NULL;
        Pvoid_t lang_freq = NULL;
        
        Word_t *f       = NULL;
        Word_t idx      = 0;
        uint   freq_lim = 0;
        uint   i, j, k;
        
        const char *ix_name    = NULL;
        const char *stats_name = NULL;
        
        dub_init();

        stats_name = pparm_common_name("istats");
        ix_name    = pparm_common_name("ixi");
        
        PPARM_INT(freq_lim, NOF_FREQUENT);
        
        if (freq_lim >= XID_TOKEN_FREQUENT_L)
                dub_die("NOF_FREQUENT must be less than %u",
                                XID_TOKEN_FREQUENT_L);
        
        load_ixicon(ix_name);
        load_istats(stats_name);
        
        dub_msg("Overwriting old %s and %s", ix_name, stats_name);
       
        qsort(istats, istats_len, sizeof(struct istat_entry), fix_cmp);

        rixicon = reverse_ixicon();

        dub_dbg("ISTATS_LEN %u", istats_len);
        
        for (k = 0, j = 0, i = 0; i < istats_len; i++){

                Word_t *val = NULL;
                idx = istats[i].xid;

                if (idx > XID_TOKEN_L)
                        continue;
       
                JLG(val, rixicon, idx);
                if (!val)
                        dub_die("No matching ixeme for ID %u", idx);
               
                JSLI(f, new_ixi, (const char*)*val);

                JLI(val, lang_freq, (Word_t)istats[i].lang_code);
               
                if (*val < freq_lim){
                        ++*val;
                        *f = ++j;

                        if (j >= XID_TOKEN_FREQUENT_L)
                                dub_die("Too many frequent ixemes. "
                                "Consider lowering NOF_FREQUENT (now %u)",
                                freq_lim);
                        
                }else
                        *f = XID_TOKEN_FREQUENT_L + ++k;

                istats[i].xid = *f;
        }

        copy_sites(new_ixi);        
       
        create_ixicon(ix_name, new_ixi);

        close_istats();
        
        return 0;
}
        
