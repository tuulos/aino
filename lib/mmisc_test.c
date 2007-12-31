
#include <stdlib.h>

#include "dub.h"
#include "mmisc.h"

#define LEN 10000

int main(int argc, char **argv)
{
        uint i;
        growing_glist *g = NULL;
            
        dub_init();

        GGLIST_INIT(g, LEN / 10);

        if (g->sze != LEN / 10)
                dub_die("INIT failed: %u != %u", g->sze, LEN / 10);
        
        for (i = 0; i < LEN; i++)
                GGLIST_APPEND(g, i);

        if (g->lst.len != LEN)
                dub_die("APPEND failed: %u != %u", g->lst.len, LEN);

        for (i = 0; i < LEN; i++)
                if (g->lst.lst[i] != i)
                        dub_die("Check failed: %u != %u", g->lst.lst[i], i);
       
        dub_dbg("Size after %u appends: %u", LEN, g->sze);
        
        return 0;
}

