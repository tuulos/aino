
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <ctype.h>

#include <Judy.h>

#include <dub.h>
#include <ixemes.h>
#include <mmisc.h>
#include <pparam.h>
#include <qexpansion.h>
#include <ixicon.h>

#include <ixeme_stats.h>

#include "snowball_lemmatize.h"

#define MAX_LEMMA_LEN 50
        
static uint max_len;
static uint longest_lemma;

static void output_lemmas(const Pvoid_t lemmas)
{
        const char *fname = getenv("OUTPUT_LEMMAS");
        char       *lemma = xmalloc(longest_lemma + 1);
        Word_t     *xid   = NULL;
        FILE       *f     = NULL;
       
        if (!(f = fopen(fname, "w+")))
                dub_sysdie("Couldn't open %s", fname);
        
        strcpy(lemma, "");
        JSLF(xid, lemmas, lemma);
        
        while(xid != NULL){
                fprintf(f, "-%d\t%s\n", ((glist*)*xid)->lst[0], lemma);
                JSLN(xid, lemmas, lemma);
        }

        fclose(f);
        
        dub_msg("Lemmas output to %s", fname);
        
        free(lemma);
}

static inline uint lemmatizable(const char *token)
{
        uint len = strlen(token);

        if (len > max_len)
                return 0;
        
        while (len--)
                if (!isalpha(token[len]))
                        return 0;
        return 1;
}

int main(int argc, char **argv)
{
        const char *ixicon_file = NULL;
        const char *istats_file = NULL;
        const char *qexp_file   = NULL;
        
        Pvoid_t    rixicon = NULL;
        Pvoid_t    lemmas  = NULL;
        Pvoid_t    qexp    = NULL;
        
        uint i;
        uint nl      = 0;
        uint ul      = 0;
        u32  xid     = 0;
        u32  freq_xid = 0;
        uint not_qexp = 0;
        
        dub_init();
        
        ixicon_file = pparm_common_name("ixi");
        istats_file = pparm_common_name("istats");
        qexp_file   = pparm_common_name("qexp");
        
        PPARM_INT(max_len, MAX_LEMMA_LEN);
        
        if (getenv("LOCALE")){
                if(!setlocale(LC_ALL, getenv("LOCALE")))
                        dub_sysdie("Couldn't set locale %s",
                                    getenv("LOCALE"));
                else
                        dub_msg("Locale set to %s", getenv("LOCALE"));
        }

        /* qexp_file might not exist; no problem */
        not_qexp = load_qexp(qexp_file, 1);

        load_ixicon(ixicon_file);
        rixicon = reverse_ixicon();

        load_istats(istats_file);
        
        snowball_init();

        for (i = 0; i < istats_len; i++){
                
                Word_t     idx    = (Word_t)istats[i].xid;
                Word_t     *val   = NULL;
                const char *lemma = NULL;
                const char *token = NULL;
                u32        lid    = 0;

                if (idx > XID_TOKEN_L)
                        continue;
                
                JLG(val, rixicon, idx);
                if (!val)
                        dub_die("Ixicon doesn't contain xid %u",
                                        istats[i].xid);
        
                token = (const char*)*val;
              
		if (istats[i].lang_code < XID_META_LANG_F || 
                    istats[i].lang_code > XID_META_LANG_L)
			continue;
		
                if (!lemmatizable(token)){
                        dub_dbg("Not lemmatizable: %s", token);
                        continue;
                }
                
                lemma = snowball_lemmatize(token, istats[i].lang_code);
	
                /* no lemma found (unknown language etc.) */
                if (!lemma)
                        continue;
                        
                JSLI(val, lemmas, lemma);

                if (!*val){

                        if (strlen(lemma) > longest_lemma)
                                longest_lemma = strlen(lemma);
                        
                        glist *lst = xmalloc(sizeof(glist) + 4);
                        lst->len = 1;
                        
                        /* unseen lemma */
                        /* if a token is frequent, its lemma is also */
                        if (idx < XID_TOKEN_FREQUENT_L){
                                
                                lid = XID_META_FREQUENT_F + freq_xid++;
                                if (freq_xid >= XID_META_FREQUENT_L)
                                        /* freq_ixicon has failed somehow */
                                        dub_die("Too many frequent ixemes");
                        }else
                                lid = XID_META_FREQUENT_L + xid++;
                       
                        if (lid > XID_META_LEMMA_L)
                                dub_die("Lemma ID range exhausted.");
                        
                        lst->lst[0] = lid;
                        *val = (Word_t)lst;
                        ++ul;
                }
                
                lid = *val;
                JLI(val, qexp, idx);
                *val = lid;
                        
                ++nl;
        }

        if (not_qexp){
                dub_msg("Qexp file %s doesn't exist. "
                        "Creating qexp from the scratch.", qexp_file);
        }else{
                dub_msg("Qexp file %s exists. Lemmas will be merged to it.",
                                qexp_file);
                
                qexp = qexp_merge(qexp);
                close_qexp();
        }
       
        create_qexp(qexp_file, qexp);
       
        if (getenv("OUTPUT_LEMMAS"))
                output_lemmas(lemmas);
        
        dub_msg("%u / %u ixemes were lemmatized", nl, istats_len);
        dub_msg("%u different lemmas were found", ul);

        return 0;
}
