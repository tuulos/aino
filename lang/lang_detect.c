/*
 *   lang/lang_detect.c
 *   Language detection with TextCat
 *   
 *   Copyright (C) 2004-2005 Ville H. Tuulos
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

#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <pparam.h>
#include <ttypes.h>
#include <dub.h>
#include <rawstream.h>
#include <ixemes.h>

#include <Judy.h>

#include <textcat.h>

#define CAT_BUF    1024
#define FIXED_LANG 0 

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

int main(int argc, char **argv)
{
        char       *cachename = NULL;
        FILE       *cache_f   = NULL;
        Pvoid_t    cache      = NULL;
        
        char       *lang_conf = NULL;
        char       *c         = NULL;
        void       *cat       = NULL;
        uint       buf_max    = 0;
        u32        fixed_lang = 0;
        
        struct rawdoc *doc;
        
        dub_init();

        PPARM_STR_D(lang_conf, LANG_CONF);
        PPARM_INT(buf_max, CAT_BUF);
        
        cachename = pparm_common_name("langs");
        if ((cache_f = fopen(cachename, "r+"))){
                dub_msg("Reading language cache %s", cachename);
                cache = read_cache(cache_f);
        }else if ((cache_f = fopen(cachename, "w+")))
                dub_msg("Creating language cache %s", cachename);
        else
                dub_sysdie("Couldn't open language cache %s", cachename);

        /* find the directory */
        c = strrchr(lang_conf, '/');
        if (c){
                *c = 0;
                if (chdir(lang_conf))
                        dub_sysdie("Couldn't chdir to %s", lang_conf);
                else
                        dub_msg("Chdir to %s", lang_conf);
                *c = '/';
        }
        dub_msg("Configuration: %s", lang_conf);
        
        if ((cat = textcat_Init(lang_conf)) == NULL)
                dub_die("Couldn't initialize text_cat");
       
       
        PPARM_INT(fixed_lang, FIXED_LANG);
        if (fixed_lang)
                dub_msg("Tagging all docs with language code %u",
                                fixed_lang);
        
        while ((doc = pull_document())){

                char *keystr = head_get_field(doc->head, "ID");
                u32 lang_id = 0;
                uint len = doc->body_size;
                char *res = NULL;
                u32 key;
                
                if (buf_max < len)
                        len = buf_max;
                
                if (fixed_lang)
                        lang_id = fixed_lang;
                
                errno = 0;
                key = strtoul(keystr, NULL, 10);
                if (!errno){
                        Word_t *val = NULL;
                        JLG(val, cache, key);
                        if (val)
                                lang_id = *val;
                        else{
                                /* res is of form "[550][551].." where the
                                 * numbers are the ixeme IDs of languages or
                                 * "[SHORT]" or "[UNKNOWN]" */
                                res = textcat_Classify(cat, 
                                        doc->body, doc->body_size);
                                /* skip the first '[' */
                                lang_id = strtoul(&res[1], NULL, 10);
                                /* write key -> language code mapping to cache */
                                fprintf(cache_f, "%s %u\n", keystr, lang_id);
                        }
                }
                
                if (lang_id >= XID_META_LANG_F && lang_id <= XID_META_LANG_L)
                        doc->head = head_meta_add(doc->head, lang_id);
               
                push_document(doc); 
                free(keystr);
        }

        fclose(cache_f);

        return 0;
}
