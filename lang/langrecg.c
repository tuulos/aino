
#include <rawstream.h>
#include <hhash.h>
#include <pparam.h>
#include <dub.h>
#include <ixemes.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <locale.h>

#define MAX_BUF    1024
#define HASHTABLE_SIZE (1 << 16)
#define TOPN 100
#define MISSING_PENALTY 500

struct freq_s{
        int idx;
        int freq;
};

struct model_s;

struct model_s{
        u32 lang_id;
        struct freq_s *ranked;
        struct model_s *next;
};

static uint *freq_table;
struct model_s *models;

static Pvoid_t read_cache(FILE *f)
{
        Pvoid_t cache = NULL;
        u32     key   = 0;
        u32     lang  = 0;
        uint    cnt   = 0;
        
        while (fscanf(f, "%u %u\n", &key, &lang) == 2){
                Word_t *val = NULL;
                JLI(val, cache, key);
                *val = lang;
                ++cnt;
        }

        dub_msg("Succesfully read %u language entries from the cache.", cnt);
        return cache;
}

static void ngram_distr(const char *txt, int len)
{
        u32 win = 0;
        const char *p = txt;
        int i = 0;

        /* skip whitespace in the beginning */
        while (!isalpha(p[i]))
                ++i;

        /* fill in the first window */
        if (len - i > 2){
                win |= (u32)'_';
                win |= ((u32)(isalpha(p[i]) ? p[i]: '_')) << 8; i++;
                win |= ((u32)(isalpha(p[i]) ? p[i]: '_')) << 16; i++;
                win |= ((u32)(isalpha(p[i]) ? p[i]: '_')) << 24; i++;
        }else
                return;

        while (1){
                
                int j;
                for (j = ((char)win == '_' ? 2: 1); j < 5; j++){
                        /*
                        int k;
                        for (k = 0; k < j; k++)
                                printf("%c", ((char*)&win)[k]);
                        printf("\n");
                        */
                        u32 hashval = hash((unsigned char*)&win, j, 101);
                        ++freq_table[hashval & (HASHTABLE_SIZE - 1)];
                        
                        if (((char*)&win)[j - 1] == '_')
                                break;
                }
                
                unsigned char c;
                do{
                        if (i >= len){
                                if (i == len + 4)
                                        return;
                                else
                                        c = '_';
                        }else
                                c = isalpha(p[i]) ? p[i]: '_';

                        win >>= 8;
                        win |= ((u32)c) << 24;
                        ++i;

                /* skip consecutive non-alphabetical characters */
                } while((char)win == '_' && ((char*)&win)[1] == '_');
                
        }
}

static int freq_cmp(const void *i1, const void *i2)
{
        return ((const struct freq_s*)i2)->freq - 
               ((const struct freq_s*)i1)->freq;
}

static int idx_cmp(const void *i1, const void *i2)
{
        return ((const struct freq_s*)i2)->idx - 
               ((const struct freq_s*)i1)->idx;
}

static struct freq_s *rank_freqtable(int *ret_len)
{
        int k, non_empty = 0;
        for (k = 0; k < HASHTABLE_SIZE; k++)
                if (freq_table[k])
                        ++non_empty;

        struct freq_s *rank_table = xmalloc(non_empty * sizeof(struct freq_s));
        int i = 0;
        for (k = 0; k < HASHTABLE_SIZE; k++)
                if (freq_table[k]){
                        rank_table[i].idx = k;
                        rank_table[i++].freq = freq_table[k];
                }

        qsort(rank_table, non_empty, sizeof(struct freq_s), freq_cmp);

        *ret_len = non_empty;
        return rank_table;
}

static int find_idx(const struct freq_s *ranked, u32 key)
{
        int  left = 0;
        int  right = TOPN;
        uint idx = 0;
        
        while (left <= right){
               
                idx = (left + right) >> 1;
              
                //printf("DD %d %u ", idx, ranked[idx].idx);

                if (ranked[idx].idx == key){
                        return ranked[idx].freq;
                }else if (ranked[idx].idx < key)
                        right = idx - 1;
                else
                        left = idx + 1;
        }
        
        return -1;
}

static uint table_dist(const struct freq_s *est, int est_len, const struct model_s *m, int best_d)
{
        int i;
        int d = 0;
        for (i = 0; i < est_len && i < TOPN; i++){
                int m_rank;
                if ((m_rank = find_idx(m->ranked, est[i].idx)) == -1){
                        d += MISSING_PENALTY;
                }else
                        d += abs(m_rank - i);

                if (d >= best_d)
                        break;
        }
        return d;
}


void load_models()
{
        FILE *f;
        char *lang_models = NULL;

        PPARM_STR_D(lang_models, LANG_MODELS);

        if (!(f = fopen(lang_models, "r+")))
                dub_sysdie("Could not open %s", lang_models);

        u32 lang_id;
        struct model_s *prev_model = NULL;

        while (fscanf(f, "%d", &lang_id) == 1){
                int i;
                struct model_s *m = xmalloc(sizeof(struct model_s));
                m->lang_id = lang_id;

                if (m->lang_id > XID_META_LANG_L || m->lang_id < XID_META_LANG_F)
                        dub_die("Lang ID %u out of XID_MET_LANG range.", lang_id);

                m->next = NULL;
                m->ranked = xmalloc(TOPN * sizeof(struct freq_s));
                memset(m->ranked, 0, TOPN * sizeof(struct freq_s));

                for (i = 0; i < TOPN; i++)
                        if (fscanf(f, "%u:%d", &m->ranked[i].idx, 
                                &m->ranked[i].freq) != 2)
                                break;
                
                if (prev_model)
                        prev_model->next = m;
                else
                        models = m;
                prev_model = m;

                dub_msg("Language model %u read with %u entries", m->lang_id, i);
        }

        fclose(f);
}

int main(int argc, char **argv)
{
        struct rawdoc *doc;
        int i;
        int max_buf = MAX_BUF;
        
        char *cachename = NULL;
        FILE *cache_f   = NULL;
        Pvoid_t cache = NULL;
        
        dub_init();
        
        PPARM_INT(max_buf, MAX_BUF);

        freq_table = xmalloc(HASHTABLE_SIZE * sizeof(int));
        memset(freq_table, 0, HASHTABLE_SIZE * sizeof(int));
        
        cachename = pparm_common_name("langs");
        if ((cache_f = fopen(cachename, "r+"))){
                dub_msg("Reading language cache %s", cachename);
                cache = read_cache(cache_f);
        }else if ((cache_f = fopen(cachename, "w+")))
                dub_msg("Creating language cache %s", cachename);
        else
                dub_sysdie("Couldn't open language cache %s", cachename);

        if (getenv("LOCALE")){
                if(!setlocale(LC_ALL, getenv("LOCALE")))
                        dub_sysdie("Couldn't set locale %s",
                                    getenv("LOCALE"));
                else
                        dub_msg("Locale set to %s", getenv("LOCALE"));
        }

        if (getenv("SAVE_MODEL")){
                
                while ((doc = pull_document()))
                        ngram_distr(doc->body, doc->body_size);

                int ranked_len;
                struct freq_s *ranked = rank_freqtable(&ranked_len);
                for (i = 0; i < ranked_len; i++)
                        ranked[i].freq = i;
               
                qsort(ranked, ranked_len, sizeof(struct freq_s), idx_cmp);

                printf("%s", getenv("SAVE_MODEL"));
                for (i = 0; i < ranked_len; i++)
                        if (ranked[i].freq < TOPN)
                                printf(" %d:%d", ranked[i].idx, ranked[i].freq);

                printf("\n");
                free(ranked);
        }else{

                load_models();

                while ((doc = pull_document())){
                
                        int closest_d = 1 << 30;
                        u32 closest_lang = 0;

                        char *keystr = head_get_field(doc->head, "ID");
                        u32 key = strtoul(keystr, NULL, 10);
                        if (!errno){
                                Word_t *val = NULL;
                                JLG(val, cache, key);
                                if (val)
                                        closest_lang = *val;
                        }
                       
                        if (!closest_lang){

                                memset(freq_table, 0, HASHTABLE_SIZE * sizeof(int));
                                if (doc->body_size > max_buf)
                                        ngram_distr(doc->body, max_buf);
                                else
                                        ngram_distr(doc->body, doc->body_size);
                                                
                                int ranked_len;
                                struct freq_s *ranked = rank_freqtable(&ranked_len);
                                const struct model_s *m = models;

                                do{
                                        int d = table_dist(ranked, ranked_len,
                                                           m, closest_d);
                                        //int d = 0;
                                        if (d < closest_d){
                                                closest_d = d;
                                                closest_lang = m->lang_id;
                                        }
                                        dub_dbg("DOC ID %s: LANG %u DIST %d\n",
                                                keystr, m->lang_id, d);
                                        m = m->next;
                                } while (m);
                                
                                fprintf(cache_f, "%s %u\n", keystr, closest_lang);
                                free(ranked);
                        }

                        dub_msg("LANG %s %u\n", keystr, closest_lang);
                        
                        doc->head = head_meta_add(doc->head, closest_lang);

                        free(keystr);
                        push_document(doc); 
                }
        }

        fclose(cache_f);

        return 0;
}

