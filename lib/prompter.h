/*
 *   lib/prompter.h
 *   A dead-simple skeleton for oriented interactive apps
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

#ifndef __PROPMTER_H__
#define __PROMPTER_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include <readline/readline.h>
#include <readline/history.h>

typedef void (action)(void);

static void _cmd_help();

static const char *cmds[] = {"help", PROMPTER_COMMANDS};
static action *actions[]  = {_cmd_help, PROMPTER_ACTIONS};

static char *args;

static inline void say(const char *fmt, ...)
{
        va_list ap;
        
        va_start(ap, fmt);
        fprintf(stderr, ">> ");
        vfprintf(stderr, fmt, ap);
        va_end(ap);
}

static void _cmd_help()
{
        uint i;
        say("Type a command name to see its description.\n");
        say("Available commands:");
        for (i = 0; i < sizeof(cmds) / sizeof(char*); i++)
                fprintf(stderr, " %s", cmds[i]);
        fprintf(stderr, "\n");
}

static void prompter(const char *pro)
{
        const char *prompt = NULL;
        char *line         = NULL;
        uint i;
        
        if (isatty(fileno(stdout)))
                prompt = pro;
        
        while((line = readline(prompt))){

                char *p = line; 
                
                while (*p && !isspace(*p))
                        ++p;
                
                /* skip empty lines and lines starting with # */
                if (*line == '#' || p == line) goto next;
                
                if (*p){
                        *p = 0;
                        ++p;
                }

                for (i = 0; i < sizeof(cmds) / sizeof(char*); i++)
                        if (!strcmp(cmds[i], line)){
                                args = p;
                                actions[i]();
                                goto next;
                        }
                
                say("Unknown command <%s>. Try 'help'.\n", line);
next:
                free(line);
        }
}

#endif /* PROMPTER_H */
