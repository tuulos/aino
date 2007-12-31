/*
 *   indexing/qexpeek.c
 *   Peek at query expansion mapping
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Judy.h>

#include <dub.h>
#include <pparam.h>
#include <mmisc.h>
#include <qexpansion.h>
#include <ixemes.h>
#include <ttypes.h>

static void reverse_lookup(u32 id)
{
        Pvoid_t qexp = judify_qexp();
        Word_t  idx  = 0;
        Word_t  *ptr = NULL;
        uint    i    = 0;
        
        JLF(ptr, qexp, idx);

        printf("ID: %d is expanded from: ", id);
        
        while (ptr != NULL){
                const glist *lst = (const glist*)*ptr;
                uint  j;
                
                for (j = 0; j < lst->len; j++){
                        if (lst->lst[j] == id){
                                printf("%u ", (u32)idx);
                                ++i;
                        }
                        if (lst->lst[j] >= id)
                                break;
                }
                
                JLN(ptr, qexp, idx);
        }
       
        if (!i)
                printf("(null)\n");
        else
                printf("\n");
}

static void print_all()
{
        Pvoid_t qexp = judify_qexp();
        Word_t  idx  = 0;
        Word_t  *ptr = NULL;

        JLF(ptr, qexp, idx);

       while (ptr != NULL){
               const glist *lst = (const glist*)*ptr;
               printf("ID: %d expands to: ", (u32)idx);
               print_glist(lst);
                
               JLN(ptr, qexp, idx);
       }
}

static void usage(){
        printf("\nUsage: qexpeek ['print_all'|[-id|-exp] token]\n\n");
        exit(1);
}

int main(int argc, char **argv)
{
        const char *fname = NULL;
        u32 id;
 
        dub_init();
         
        if (argc < 2)
                usage();

        fname = pparm_common_name("qexp");        
        load_qexp(fname, 0);
       
        if (!strcmp(argv[1], "print_all")){
                print_all();
                return 0;
                
        }else if(argc < 3)
                usage();
        
        id = atoi(argv[2]);

        if (!strcmp(argv[1], "-id")){
                const glist *exp = get_expansion(id);
                printf("ID: %d Expands to: ", id);
                if (exp)
                        print_glist(exp);
                else
                        printf("Nothing.\n");     
        }else if (!strcmp(argv[1], "-exp")){
                reverse_lookup(id);
        }else
                printf("Unknown flag: %s\n", argv[1]);

        return 0;
}

         

