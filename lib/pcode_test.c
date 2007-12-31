
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dub.h"
#include "ttypes.h"
#include "pcode.h"
#include "pparam.h"

#define SEED   1234
#define ROUNDS 1000
#define MAXSZE 10000

static inline void print(const u32 *p, uint len)
{
        uint i;
        fprintf(stderr, "Seq >>");
        for (i = 0; i < len; i++)
                fprintf(stderr, "%u ", p[i]);
        fprintf(stderr, "\n");
}

static int cmp(const void *e1, const void *e2)
{
        const u32 *i1 = (u32*)e1;
        const u32 *i2 = (u32*)e2;

        if ((*i1) > (*i2))
                return 1;
        else if((*i1) < (*i2))
                return -1;
        return 0;
}

int main(int argc, char **argv)
{
        uint bits = 0;
        uint i = ROUNDS;
        uint seed;
        char  *buf  = xmalloc(MAXSZE * 6);
        u32 *dest = NULL;
        uint dest_len = 0;
        
        dub_init();
        
        PPARM_INT(seed, SEED);

        srand(seed);
        
        while (i--){

                uint  sze  = MAXSZE * (rand() / (RAND_MAX + 1.0));
                u32 *p   = xmalloc(sze * 4);
                u32   offs = 0;
                u32 val  = 0;
                uint  j, k;
                
                for (j = 0; j < sze; j++)
                        p[j] = 1 + 1000 * (rand() / (RAND_MAX + 1.0));

                qsort(p, sze, 4, cmp);
                
                memset(buf, 0, MAXSZE * 6);

                bits += pc_encode(buf, &offs, p, sze);
               
                offs = k = 0;
                
                PC_FOREACH_VAL(buf, &offs, val)
                        
                        if (val != p[k]){
                                print(p, sze);
                                dub_die("FOREACH failed. %u != %u (idx: %u)", 
                                                val, p[k], k);
                        }
                         
                        ++k;
                        
                PC_FOREACH_VAL_END
               
                if (k != sze){
                        print(p, sze);
                        dub_die("FOREACH failed. Read only %u / %u entries.",
                                        k, sze);
                }
                        
                offs = 0;
                k = pc_decode(&dest, &dest_len, buf, &offs);

                if (k != sze){
                        print(p, sze);
                        dub_die("Decode failed. Read only %u / %u entries.",
                                        k, sze);
                }
                        
                while (k--){
                        if (dest[k] != p[k]){
                                print(p, sze);
                                dub_die("Decode failed. %u != %u (idx: %u)", 
                                                val, p[k], k);
                        }
                }
                
                dub_dbg("%u entries read ok.", sze);
                        
                free(p);
        }

        printf("%u bits used\n", bits);

        return 0;
}
