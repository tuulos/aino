/*
 *   indexing/ixpeek.c
 *   Peek at ixicon
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

#include <ixicon.h>
#include <pparam.h>
#include <dub.h>

#include <Judy.h>

int main(int argc, char **argv)
{
        const char *fname = NULL;
        
        dub_init();

        fname = pparm_common_name("ixi");

        if (argc < 2){
                printf("\nUsage: ixpeek [-id xid|-ix token|-list]\n\n");
                exit(1);
        }

        load_ixicon(fname);
        
        if (!strcmp(argv[1], "-id")){
                if (argc < 3){
                        printf("Please specify a token\n");
                        exit(1);
                }
                printf("Ixeme: %s ID: %d\n", argv[2], get_xid(argv[2]));
                
        }else if (!strcmp(argv[1], "-ix")){
                if (argc < 3){
                        printf("Please specify an ixeme ID\n");
                        exit(1);
                }
                u32 xid = atoi(argv[2]);
                printf("ID: %d Ixeme: %s\n", xid, get_ixeme(xid));
                
        }else if (!strcmp(argv[1], "-list")){
                Pvoid_t ixi = reverse_ixicon();
                Word_t xid  = 0;
                Word_t *tok = NULL;

                JLF(tok, ixi, xid);
                while(tok){
                        printf("%u %s\n", (u32)xid, (char*)*tok);
                        JLN(tok, ixi, xid);
                }
        
        }else
                printf("Unknown flag: %s\n", argv[1]);

        return 0;
}

