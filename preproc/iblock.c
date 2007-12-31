/*
 *   preproc/iblock.c
 *   
 *   Copyright (C) 2006-2008 Ville H. Tuulos
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
#include <stdlib.h>

#include <dub.h>
#include <pparam.h>

#include "iblock.h"

#include <dexvm.h>

#define IO_BUF_SIZE 1048576 /* 1MB */ 

FILE *toc_f;
FILE *data_f;
uint iblock_left;
uint iblock_cnt;
uint iblock_size;

static char *basename;

void init_iblock(const char *sect_str)
{
        PPARM_INT_D(iblock_size, IBLOCK_SIZE);
        iblock_left = 0;
        iblock_cnt  = 0;
        
        basename = pparm_common_name(sect_str);
}

void close_iblock()
{
        toc_e toc;
        
        /* write terminal TOC entry */
        toc.offs = ftello64(data_f);
        toc.val  = 0;
        fwrite(&toc, sizeof(toc_e), 1, toc_f);
        
        fclose(toc_f);
        fclose(data_f);
}

void new_iblock()
{
        char *dname;
        char *tname;

        if (toc_f)
                close_iblock();

        asprintf(&dname, "%s.%u.sect", basename, iblock_cnt);
        asprintf(&tname, "%s.%u.toc.sect", basename, iblock_cnt);

        if (!(data_f = fopen64(dname, "w+")))
                dub_sysdie("Couldn't open section %s", dname);
        if (!(toc_f = fopen64(tname, "w+")))
                dub_sysdie("Couldn't open section %s", tname);
        
        if (setvbuf(data_f, NULL, _IOFBF, IO_BUF_SIZE))
                dub_sysdie("Couldn't open output buffer");
        if (setvbuf(toc_f, NULL, _IOFBF, IO_BUF_SIZE))
                dub_sysdie("Couldn't open output buffer");

        free(dname);
        free(tname);
        
        iblock_left = iblock_size;
        ++iblock_cnt;
}
