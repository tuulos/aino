/*
 *   lib/mmisc.c
 *   Miscellaneous macros etc.
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
#include <errno.h>

#include "dub.h"
#include "mmisc.h"

glist *str2glist_len(char **str, u32 len)
{
        char       *tok = (char*)*str;
        u32        val  = 0;
        glist      *lst = NULL;
        uint       i    = 0;


        lst      = xmalloc(len * 4 + sizeof(glist));
        lst->len = len;
       
        if (!len)
                goto end;
        
        while (i < len){
               val = strtoul(tok, &tok, 10);
               if (errno)
                       dub_sysdie("Invalid list <%s>", *str);
               
               lst->lst[i++] = val;
        }

        if (i < len)
                dub_die("List too short <%s>", *str);

end:
        *str = tok;
        
        return lst;

}

glist *str2glist(char **str)
{
        char *tok = (char*)*str;
        u32  len  = 0;
       
        errno = 0;
        len   = strtol(*str, &tok, 10);
       
        if (len < 0 || errno)
                dub_sysdie("Invalid list length, list: <%s>", *str);

        return str2glist_len(&tok, len);
}

void print_glist(const glist *lst)
{
        uint i;

        for (i = 0; i < lst->len; i++)
                printf("%u ", lst->lst[i]);
        printf("\n");
}

/* the lists must be ascending */
/* removes duplicates */
glist *merge_glists(const glist *lst1, const glist *lst2)
{
        return merge_glists_g(lst1->lst, lst1->len,
                              lst2->lst, lst2->len);
}

glist *merge_glists_g(const u32 *lst1, uint len1, 
                      const u32 *lst2, uint len2)
{
        glist *new = (glist*)xmalloc(sizeof(glist) + (len1 + len2) * 4);
        u32  ni, i1, i2;
        
        new->len = len1 + len2;

        ni = i1 = i2 = 0;
        
        while (ni < new->len){
               
                if (i2 == len2){
                        new->lst[ni++] = lst1[i1++];
                        continue;
                }
                
                if (i1 == len1){
                        new->lst[ni++] = lst2[i2++];
                        continue;
                }

                if (lst1[i1] == lst2[i2]){
                        new->lst[ni++] = lst1[i1++];
                        --new->len;
                        ++i2;
                }else if (lst1[i1] < lst2[i2])
                        new->lst[ni++] = lst1[i1++];
                else
                        new->lst[ni++] = lst2[i2++];
        }

        return new;
}

#ifdef TEST
int main(int argc, char **argv)
{
        u32 lst1[] = {1,3,5,40};
        u32 lst2[] = {1,4,5,5,5,60,600,700};
        glist *f1 = xmalloc(sizeof(glist) + sizeof(lst1));
        glist *f2 = xmalloc(sizeof(glist) + sizeof(lst2));
        glist *res = NULL;
        uint i;
        
        f1->len = sizeof(lst1) / 4;
        memcpy(f1->lst, lst1, sizeof(lst1));
        f2->len = sizeof(lst2) / 4;
        memcpy(f2->lst, lst2, sizeof(lst2));

        printf("LEN1 %u LEN2 %u\n", f1->len, f2->len);
        
        res = merge_glists(f2, f1);
        
        for (i = 0; i < res->len; i++)
                printf("%u ", res->lst[i]);
        printf("\n");

        return 0;
}
#endif
