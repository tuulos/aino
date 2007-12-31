
#include <stdlib.h>

#include "bbitmap.h"

int main(int argc, char **argv)
{
        uint i;
        for (i = 1; i < 5000; i++){
                uint r, j = i;
                bmap_t *b = bmap_init(i);
                while (j--){
                        
                        if (bmap_test(b, j))
                                dub_die("False positive (%u, %u)", i, j);
                        
                        bmap_set(b, j);
                        if (!bmap_test(b, j))
                                dub_die("False negative (%u, %u)", i, j);
                        
                        if ((r = bmap_first_set(b)) != j + 1)
                                dub_die("ffs failed: %u (%u, %u)", r, i, j);
                        
                        if (j > 0){
                                if ((r = bmap_first_unset(b)) != 1)
                                        dub_die("nffs failed: %u (%u, %u)", 
                                                        r, i, j);
                        }else{
                                if ((r = bmap_first_unset(b)) != 2)
                                        dub_die("nffs failed (2): "
                                                "%u (%u, %u)", r, i, j);
                        }
                        
                        bmap_unset(b, j);
                        if (bmap_test(b, j))
                                dub_die("False positive (2) (%u, %u)", i, j);
                }
                free(b);
        }
                
        return 0;
}
