
#include <stdlib.h>
#include <stdio.h>

#include <Judy.h>

#include <ttypes.h>
#include <dub.h>
#include <bloom.h>
#include <pparam.h>

#define SEED   123
#define ROUNDS 1000
#define MAXSZE 10000
#define TESTMAX 1000000

#define SCALER 1.0

static inline void print(const u32 *p, uint len)
{
        uint i;
        fprintf(stderr, "Seq >>");
        for (i = 0; i < len; i++)
                fprintf(stderr, "%u ", p[i]);
        fprintf(stderr, "\n");
}


int main(int argc, char **argv)
{
        uint  r          = ROUNDS;
        uint  only_crea  = 0;
        uint  do_judy    = 0;
        uint  seed       = 0;
        float scaler     = 0;
        uint  sum        = 0;
        uint  false_hits = 0;
        uint  nof_false  = 0;
        
        Pvoid_t judy     = NULL;
        
        dub_init();

        PPARM_INT(seed, SEED);
        PPARM_FLOAT(scaler, SCALER);
        
        /* allow at most one false hit in 10^7 queries */
        init_bloom(7, scaler);

        if (getenv("ONLY_CREATE")){
                dub_msg("Only encoding %u lists", r);
                only_crea = 1;
        }else
                dub_msg("Encoding %u lists and testing %u items per list",
                                r, TESTMAX);
        
        if (!only_crea && getenv("FALSE_HITS")){

                dub_msg("Checking for false hits");
                false_hits = 1;
                
        }else if (getenv("JUDY")){
                dub_msg("Using Judy");
                do_judy = 1;
        }
        
        srand(seed);

        while (r--){
                
                uint  sze  = MAXSZE * (rand() / (RAND_MAX + 1.0));
                u32   *p   = xmalloc(sze * 4);
                bloom_s *b = NULL;
                uint  j, k;
                
                for (j = 0; j < sze; j++)
                        p[j] = 1000 * (rand() / (RAND_MAX + 1.0));

                
                if (do_judy || false_hits){
                        uint tmp;
                        for (j = 0; j < sze; j++)
                                J1S(tmp, judy, p[j]);
                }

                if (!do_judy)
                        b = new_bloom(p, sze);

                sum += sze;
                
                if (only_crea)
                        goto next;
                
                for (j = 1; j < TESTMAX; j++){

                        if (false_hits){
                                
                                J1T(k, judy, j);
                                if (bloom_test(b, j)){
                                        if (!k) ++nof_false;
                                }else
                                        if (k){
                                                print(p, sze);
                                                dub_die("False negative! "
                                                  "Value %u not in bloom. "
                                                   "Bloom broken!", j);
                                        }
                                        
                        }else if (do_judy){
                                J1T(k, judy, j);
                        }else
                                bloom_test(b, j);
                }
                
next:
                if (do_judy || false_hits){
                        J1FA(k, judy);                
                }
                
                if (!do_judy){
                        free(b);
                        free(p);
                }
        }
        
        dub_msg("In total %u items in lists", sum);
        if (false_hits)
                dub_msg("In total %u false hits found", nof_false);
        
        return 0;
}
