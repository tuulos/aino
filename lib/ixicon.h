/*
 *   lib/ixicon.h
 *   Maintains ixeme->ix_id mapping
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

#ifndef __IXICON_H__
#define __IXICON_H__

#include <Judy.h>
#include "ttypes.h"

#define MAX_TOK_LEN 255

void create_ixicon(const char *fname, const Pvoid_t lexicon);
void load_ixicon(const char *fname);
Pvoid_t judify_ixicon();
Pvoid_t reverse_ixicon();
void copy_sites(Pvoid_t judy);

u32 get_xid(const char* ixeme);

/* heavy operation -- use only for debugging */
const char *get_ixeme(u32 xid);

#endif /* __IXICON_H__ */

