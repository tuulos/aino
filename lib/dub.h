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

#ifndef _DUB_H_
#define _DUB_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int dub_debug;
extern int dub_verbose;
extern char *dub_src_file;
extern int dub_src_line;

void dub_dbg0(const char *fmt, ...);

#ifdef DEBUG
#define dub_dbg(...)  (dub_src_file = __FILE__,\
                       dub_src_line = __LINE__,\
                       dub_dbg0(__VA_ARGS__))
#else
#define dub_dbg(...)   dub_dbg0(__VA_ARGS__) 
#endif
        
void dub_init();
void dub_file(const char *name);
void dub_warn(const char *fmt, ...);
void dub_sysmsg(const char *fmt, ...);
void dub_msg(const char *fmt, ...);
void dub_sysdie(const char *fmt, ...);
void dub_die(const char *fmt, ...);

void *xmalloc0(unsigned int size);
        
#ifdef DEBUG
#define xmalloc(s)  (dub_src_file = __FILE__,\
                     dub_src_line = __LINE__,\
                     xmalloc0(s))
#else
#define xmalloc(s) xmalloc0(s)
#endif
        
#ifdef __cplusplus
}
#endif

#endif /* DUB_H */
