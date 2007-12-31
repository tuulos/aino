/*
 *   preproc/build_index.c
 *   Glues the index sections together
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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include <dub.h>
#include <rrand.h>
#include <ttypes.h>
#include <pparam.h>
#include <finnuio.h>

#define DEX_NAMES
#include <dexvm.h>
//#include "defaults.h"

uint iblock;
uint segment_size;

static u64 append_sect(int dest_fd, u64 offs, enum dex_sect_id sect, uint toc)
{
        char    *basename = pparm_common_name(dex_names[sect]);
        char    *name    = NULL;
        int     fd        = 0;

        struct stat64 nfo;

        if (toc)
                asprintf(&name, "%s.%u.toc.sect", basename, iblock);
        else
                asprintf(&name, "%s.%u.sect", basename, iblock);

        if ((fd = open(name, O_RDONLY | O_LARGEFILE, 0)) == -1)
                dub_sysdie("Couldn't open file %s", name);

        if (fstat64(fd, &nfo))
                dub_sysdie("Couldn't stat %s", name);
       
        if (nfo.st_size > UINT32_MAX)
                dub_die("Section %s is over 4GB! Index can't be built. "
                        "Try to decrease IBLOCK_SIZE.", name);
        
        if (dest_fd)
                fio_copy_file(dest_fd, offs, fd, nfo.st_size);
        
        close(fd);
        free(name);
        free(basename);
        
        return nfo.st_size;

}

static void print_structure(const struct header_s *h)
{
        uint i;

        printf("\n  INDEX %s/%u\n\n", getenv("NAME"), iblock);
        printf("  offset        section\n");
        printf(" ------------------------------------HEADER-\n");
        printf(" | %-*llu | header\n", 13, 0LLU);
        printf(" ---------------------------------------TOC-\n");

        for (i = 0; i < DEX_NOF_SECT; i++)
                printf(" | %-*llu | TOC: %s\n", 13, h->toc_offs[i],
                        dex_names[i]);
        
        printf(" --------------------------------------DATA-\n");
        
        for (i = 0; i < DEX_NOF_SECT; i++)
                printf(" | %-*llu | %s\n", 13, h->data_offs[i],
                        dex_names[i]);
        
        printf(" -------------------------------------------\n");
}

static void fill_parameters(struct header_s *h)
{
        rrand_seed(time(NULL), time(NULL) + 2);
        
        h->index_id = rrand();
        h->iblock = iblock;
        h->segment_size = segment_size;

        char *basename = pparm_common_name(dex_names[FW]);
        char *name;
        int fd;

        asprintf(&name, "%s.%u.nfo", basename, iblock);
        if ((fd = open(name, O_RDONLY, 0)) == -1)
                dub_sysdie("Couldn't open file %s", name);

        fio_read(fd, h->fw_layers, sizeof(h->fw_layers));
        close(fd);

        free(name);
        free(basename);
}

int main(int argc, char **argv)
{
        u64  total_size = sizeof(struct header_s);
        char *basename  = pparm_common_name("index");
        char *iname     = NULL;
        int  fd         = 0;
        u64  offs       = sizeof(struct header_s);
        uint i;
       
        struct header_s header;
        
        dub_init();
      
        PPARM_INT_D(iblock, IBLOCK);
        PPARM_INT_D(segment_size, SEGMENT_SIZE);

        asprintf(&iname, "%s.%u", basename, iblock);
        
        fill_parameters(&header);
        
        for (i = 0; i < DEX_NOF_SECT; i++){
                header.toc_offs[i] = total_size;
                total_size += append_sect(0, 0, i, 1);
        }
        for (i = 0; i < DEX_NOF_SECT; i++){
                header.data_offs[i] = total_size;
                total_size += append_sect(0, 0, i, 0);
        }

        if ((fd = open(iname, O_CREAT | O_TRUNC | O_RDWR | O_LARGEFILE,
                                        S_IREAD | S_IWRITE)) == -1)
                dub_sysdie("Couldn't open file %s", iname);
        
        if (fio_truncate(fd, total_size))
                dub_sysdie("Couldn't truncate index to %llu bytes",
                                total_size);
        
        write(fd, &header, sizeof(struct header_s));
        
        for (i = 0; i < DEX_NOF_SECT; i++)
                offs += append_sect(fd, offs, i, 1);
        
        for (i = 0; i < DEX_NOF_SECT; i++)
                offs += append_sect(fd, offs, i, 0);

        close(fd);
        
        print_structure(&header);
        printf(" In total %llu bytes.\n\n", total_size);
        
        return 0;
}
