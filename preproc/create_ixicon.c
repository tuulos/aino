
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include <Judy.h>

#include <dub.h>
#include <ixicon.h>
#include <ixemes.h>
#include <pparam.h>

#define REMOVE_PREFIX 0

int main(int argc, char **argv)
{
        Pvoid_t ixicon = NULL;
        char *tok = NULL;
        unsigned int xid, remove_prefix = 0;
        FILE *f;
        
        dub_init();

        PPARM_INT(remove_prefix, REMOVE_PREFIX);

        if (argc < 3)
                dub_die("Usage: create_ixicon <netstring_ixicon> <new_ixicon>");

        if (!(f = fopen(argv[1], "r")))
                dub_die("Couldn't open %s", argv[1]);

        if (fscanf(f, "%d\n", &xid) != 1)
                dub_die("Invalid header in %s", argv[1]);

        while(fscanf(f, "%*d %as %*d %d", &tok, &xid) == 2){
                Word_t *ptr;
                if (remove_prefix && xid >= XID_META_F &&
                    xid <= XID_META_L && tok[1] == ':'){
                        JSLI(ptr, ixicon, &tok[2]);
                }else{
                        JSLI(ptr, ixicon, tok);
                }
                *ptr = xid;
                free(tok);
        }
        create_ixicon(argv[2], ixicon);
        dub_msg("New ixicon created in %s", argv[2]);
        return 0;
}
