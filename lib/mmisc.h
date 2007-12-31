/*
 *   lib/mmisc.h
 *   Miscellaneous macros etc.
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

#ifndef __MMISC_H__
#define __MMISC_H__

#include <dub.h>
#include <gglist.h>

#define PAGE_MASK          (~(getpagesize() - 1))
#define PAGE_ALIGN(addr)   ((intptr_t)((addr) + getpagesize() - 1) & PAGE_MASK)

glist *str2glist(char **str);
glist *str2glist_len(char **str, u32 len);
glist *merge_glists(const glist *lst1, const glist *lst2);
glist *merge_glists_g(const u32 *lst1, uint len1, 
                      const u32 *lst2, uint len2);
void print_glist(const glist *lst);

#endif /* __MMISC_H__ */
