/*
 *   lib/uurl.h
 *   URL helpers
 *   
 *   Copyright (C) 2005-2008 Ville H. Tuulos
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

#include <string.h>
#include <stdlib.h>
#include <dub.h>
#include <ctype.h>

#include "ttypes.h"
#include "uurl.h"
#include "hhash.h"

char *url_get_site(const char *url, uint keep_www)
{
        char *p  = NULL;
        char *t  = NULL;
        uint len = 0;
        
        if (!url || !url[0])
                return NULL;
       
        /* skip possible protocol specifier in the beginning */
        if (!strncasecmp(url, "http://", 7))
                url += 7;
        
        /* Skip embedded username & password, 
         * like in http://login:passwd@foo.bar */
        if ((t = index(url, '@'))){
                char *c = index(url, '?');
                if (!c || c > t)
                        url = t + 1;
        }
        
        if (!keep_www){
                /* skip www in the beginning */
                if (!strncasecmp(url, "www", 3))
                        url += 3;
        }

        /* look for site endings in URL */
        len = strcspn(url, "/?:");
       
        p = xmalloc(len + 1);
        memcpy(p, url, len);
        p[len] = 0;
       
        /* if the result doesn't contain a dot, we screwed up */
        if (index(p, '.'))
                return p;
        else
                return NULL;
}

u32 url_site_hash(const char *url)
{
        char *p;
        char *site = p = url_get_site(url, 0);
        uint len   = 0;
        u32  id    = 0;
        
        if (!site)
                return 0;
       
        while (*p){
                *p = tolower(*p);
                ++p;
                ++len;
        }

        id = hash(site, len, 25); /* 25 is a magic value */
        
        free(site);
        return id;
}

#if 0
#include <stdio.h>

int main(int argc, char **argv)
{
        dub_init();
        printf("SITE <%s>\n", url_get_site(argv[1]));
        return 0;
}
#endif
