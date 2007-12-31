/*
 *   DUB -- A logging library based on YAP
 *   
 *   Copyright (C) 2004-2008 Ville H. Tuulos
 *   Copyright (C) 2002 Wray Buntine
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
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "dub.h"

/*  flags for dub_print() */
#define DUB_ERR 2
#define DUB_STD 4

static char *dub_stamp();
static void dub_print(int flag, const char *fmt, va_list ap);
void dub_sysdie(const char *fmt, ...);

/*
 *  Error logging by default goes to standard error.
 */
static int  dub_fd = STDERR_FILENO;
int  dub_debug   = 0;
int  dub_verbose = 0;
int  dub_flush   = 0;

char  *dub_src_file = NULL;
int   dub_src_line  = 0;

void dub_init()
{
        if (getenv("DUB_DEBUG"))
                dub_debug = 1;

        if (getenv("DUB_VERBOSE"))
                dub_verbose = 1;

        if (getenv("DUB_FILE"))
                dub_file(getenv("DUB_FILE"));
        
        if (getenv("DUB_FLUSH"))
                dub_flush = 1;
}

void *xmalloc0(unsigned int size)
{
       void *p = malloc(size);
       if (!p)
               dub_die("Memory allocation failed (%u bytes)", size);
       return p;
}

/*
 * Use write()s to this fd to report errors.
 * Safe to call if simply reopening.
 * name = name of error file
 */
void dub_file(const char *name){
        int fd;
  
        /*  put error log into safe mode beforehand */
        if (dub_fd > 0 && dub_fd != STDERR_FILENO){
            close(dub_fd);
            dub_fd = STDERR_FILENO;
        }
  
        if ((fd = open(name, O_CREAT | O_WRONLY | O_APPEND, 0600)) < 0)
            dub_sysdie("ERROR: opening %s", name);
  
        dub_fd = fd;
}


/* Unconditional warning */
void dub_warn(const char *fmt, ...)
{
        va_list ap;
        
        va_start(ap, fmt);
        dub_print(DUB_STD, fmt, ap);
        va_end(ap);
}

/*
 * Nonfatal error related to a system call.
 */
void dub_sysmsg(const char *fmt, ...)
{
        va_list ap;
        
        if (!dub_verbose) return;

        va_start(ap, fmt);
        dub_print(DUB_ERR, fmt, ap);
        va_end(ap);
}


/*
 * Nonfatal message
 */
void dub_msg(const char *fmt, ...)
{
        va_list ap;

        if (!dub_verbose) return;

        va_start(ap, fmt);
        dub_print(DUB_STD, fmt, ap);
        va_end(ap);
}

void dub_dbg0(const char *fmt, ...)
{
        va_list ap;

        if (!dub_debug) return;

        va_start(ap, fmt);
        dub_print(DUB_STD, fmt, ap);
        va_end(ap);
}

/*
 * Fatal error related to a system call.
 */
void dub_sysdie(const char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        dub_print(DUB_ERR, fmt, ap);
        va_end(ap);
  
        abort();
}


/*
 * Fatal error unrelated to a system call.
 */
void dub_die(const char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        dub_print(DUB_STD, fmt, ap);
        va_end(ap);

        abort();
}

/*
 * Return static string containing current time.
 */
static char *dub_stamp(void)
{
        static char   timestring[35];
        struct tm     *tmp;
        time_t        currt = time(NULL);
        static pid_t pid = 0;

        if (!pid)
                pid = getpid();

        tmp = localtime(&currt);
  
        snprintf(timestring, 35, "[%02d/%02d/%4d:%02d:%02d:%02d|%u] ", 
	  tmp->tm_mday,tmp->tm_mon,1900 + tmp->tm_year, 
	  tmp->tm_hour,tmp->tm_min, tmp->tm_sec, pid);
 
        return timestring;
}

static void dub_print(int flag, const char *fmt, va_list ap)
{
        int          errno_save;
        static char  buf[10240];
        
        assert(dub_fd > 0);
        buf[0] = 0;

        errno_save = errno;         
        
        strcpy(buf, dub_stamp());

        if (flag & DUB_ERR)
                snprintf(buf + strlen(buf), 10240 - strlen(buf), "%s: ",
                                strerror(errno_save));
        
        vsnprintf(buf + strlen(buf), 10240 - strlen(buf), fmt, ap);

        if (dub_src_file) {
                snprintf(buf + strlen(buf), 10240 - strlen(buf), " (%s:%d)",
                         dub_src_file, dub_src_line);
                dub_src_file = NULL;
                dub_src_line = 0;
        }
        
        snprintf(buf + strlen(buf), 10240 - strlen(buf), "\n");
        
        write(dub_fd, buf, strlen(buf));

        if (dub_flush)
                fsync(dub_fd);
        
        errno = errno_save;
}

