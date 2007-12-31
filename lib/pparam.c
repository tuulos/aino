/*
 *   lib/pparam.c
 *   Parameters via environment
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

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

#include "pparam.h"

char *pparm_common_name(const char *postfix)
{
        char *base = getenv("NAME");
        char *str  = NULL;

        if (!base)
                dub_die("NAME not specified");
        
        if (asprintf(&str, "%s.%s", base, postfix) < 0)
                dub_die("Couldn't form name <%s.%s>", base, postfix);
        
        return str;
}

void pparm_read_env(const char *fname)
{
        static char line[LINEMAX + 1];
        FILE *f = NULL;
        
        if (!(f = fopen(fname, "r"))){
                fprintf(stderr, "Couldn't open environment %s\n", fname);
                exit(1);
        }
       
        while (fgets(line, LINEMAX, f)){
                
                /* get rid of the newline */
                if (line[strlen(line) - 1] == '\n' ||
                    line[strlen(line) - 1] == EOF)

                        line[strlen(line) - 1] = 0;
                else{
                        fprintf(stderr, "A line is longer than %u\n", LINEMAX);
                        exit(1);
                }

                if (line[0] == '#')
                        continue;
                
                char *delim = strchr(line, '=');
                if (!delim)
                        continue;

                *delim = 0;
                setenv(line, delim + 1, 0);
        }

        fclose(f);
}
