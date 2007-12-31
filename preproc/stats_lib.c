/*
 *   preproc/stats_lib.c
 *   Write ixeme statistics to disk
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <dub.h>
#include <mmisc.h>

#include "ixeme_stats.h"

uint               istats_len;
struct istat_entry *istats;

static uint size;

/* entries-array should contain only positive keys */ 
uint write_istats(const char *file, Pvoid_t entries)
{
        Word_t *e  = NULL;
        Word_t idx = 0;
        uint ic    = 0;
        uint size  = 0;
        void *p    = NULL;
        void *orig = NULL;
        int fd;

        JLC(ic, entries, 0, -1);
        size = sizeof(uint) + ic * sizeof(struct istat_entry);
        
        if ((fd = open(file, O_CREAT | O_TRUNC | O_RDWR,
                                        S_IREAD | S_IWRITE)) == -1)
                dub_sysdie("Couldn't open stats file %s", file);
        
        if (ftruncate(fd, PAGE_ALIGN(size)))
                dub_sysdie("Couldn't truncate %s to %u bytes.", file, size);

        orig = p = mmap(0, PAGE_ALIGN(size), PROT_WRITE, MAP_SHARED, fd, 0);
        if (p == MAP_FAILED)
                dub_sysdie("Couldnt mmap %s", file);
       
        memcpy(p, &ic, sizeof(uint));
        p  += sizeof(uint); 
        
        JLF(e, entries, idx);
        while (e != NULL){

                memcpy(p, (struct istat_entry*)*e, sizeof(struct istat_entry));
                p += sizeof(struct istat_entry);
                
                JLN(e, entries, idx);
        }
        
        if (munmap(orig, size))
                dub_sysdie("Couldn't munmap stats. Disk full?");
        
        close(fd);
        
        return ic;
}

void load_istats(const char *file)
{
        struct stat  f_nfo;
        int          fd = 0;
        void         *p = NULL;  
        
        if ((fd = open(file, O_RDWR)) == -1)
                dub_sysdie("Couldn't open stats file %s", file);

        if (fstat(fd, &f_nfo))
                dub_sysdie("Couldn't stat stats file %s", file);
        
        size = PAGE_ALIGN(f_nfo.st_size);
        
        p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (p == MAP_FAILED)
                dub_sysdie("Couldn't mmap stats file %s", file);

        istats_len = *(uint*)p;
        p += sizeof(uint);

        istats = p;
}

void close_istats()
{
        munmap(istats, size);
}
