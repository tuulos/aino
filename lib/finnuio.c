/*
 *   lib/finnuio.c
 *   Some IO related operations
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

#include <sys/mman.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "finnuio.h"
#include "dub.h"

/* must be a multiple of page size */
#define COPY_BLOCK 104857600 /* 10Mb */

int fio_write(int fd, const void *buf, uint blen)
{
        uint i = 0;
        while(i < blen){
                int len = write(fd, buf, blen - i);
                if (len <= 0) return len;
                i   += len;
                buf += len;
        }
        return blen;
}

int fio_read(int fd, void *buf, uint blen)
{
        uint i = 0;
        while(i < blen){
                int len = read(fd, buf, blen - i);
                if (len <= 0) return len;
                i   += len;
                buf += len;
        }
        return blen;
}

int fio_truncate(int fd, u64 sze)
{
        struct statvfs64 stat;
        int           ret = 0;

        if ((ret = fstatvfs64(fd, &stat)))
                return ret;

        if (stat.f_bavail * stat.f_bsize < sze){
                errno = EFBIG;
                return -1;
        }
        
        if ((ret = ftruncate64(fd, sze)))
                return ret;

        return 0;
}

void fio_copy_file(int dest_fd, u64 dest_offs, int src_fd, u64 src_sze)
{
        u64 offs    = 0;
        u64 o_align = dest_offs & (getpagesize() - 1);
        
        if (lseek64(src_fd, 0, SEEK_SET) < 0)
                dub_sysdie("Seeking failed");
       
        while (offs < src_sze){
                
                void *src = NULL;
              
                void *dest = mmap64(0, COPY_BLOCK + o_align, PROT_WRITE,
                                       MAP_SHARED, dest_fd,
                                       (dest_offs - o_align) + offs);
                
                if (dest == MAP_FAILED)
                        dub_sysdie("Couldn't mmap destination");
                
                dest += o_align;
                
                src = mmap64(0, COPY_BLOCK, PROT_READ, MAP_SHARED,
                                src_fd, offs);
                
                if (src == MAP_FAILED)
                        dub_sysdie("Couldn't mmap source");
                
                memcpy(dest, src,
                        (src_sze - offs > COPY_BLOCK ? COPY_BLOCK:
                         src_sze - offs));

                munmap(src,  COPY_BLOCK);
                
                if(munmap(dest - o_align, COPY_BLOCK + o_align))
                        dub_sysdie("Couldn't munmap the destination. "
                                   "Disk full?");

                offs += COPY_BLOCK;
        }
}

glist *fio_read_glist(FILE *in_f)
{
        int   ret  = 0;
        u32   len  = 0;
        glist *lst = NULL;

       if ((ret = fread(&len, 4, 1, in_f)) != 1)
               dub_sysdie("Couldn't read list length");

       lst = xmalloc(len * 4 + sizeof(glist));
       
       lst->len = len;
      
       if (lst->len && fread(lst->lst, 4, len, in_f) < len)
               dub_sysdie("Couldn't read a list");
       
       return lst;
}

#ifdef TEST

#include <fcntl.h>

int main(int argc, char **argv)
{
        int fd;
 
        dub_init();
        
        if ((fd = open("/tmp/__trunc_test", O_CREAT | O_RDWR, 
                                            S_IREAD | S_IWRITE)) == -1)
                dub_sysdie("Couldn't open test file /tmp/__trunc_test");

        /* try to allocate a terabyte */
        if (fio_truncate(fd, 1024UL*1024*1024*1024))
                dub_sysdie("Couldn't truncate a terabyte file. That's fine!");

        close(fd);
}

#endif

