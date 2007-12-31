/*
 *   preproc/ixemes_stats.h
 *   Write ixeme statistics to disk
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


#ifndef __IXEME_STATS_H__
#define __IXEME_STATS_H__

#include <ttypes.h>
#include <Judy.h>

struct istat_entry{
        u32    xid;  
        uint   freq;
        u32    lang_code;
} __attribute__((packed));

extern uint               istats_len;
extern struct istat_entry *istats;

uint write_istats(const char *file, Pvoid_t entries);
void load_istats(const char *file);
void close_istats();

#endif /* __IXEME_STATS_H__ */
