/*
 *   lib/qexpansion.h
 *   Query expansions
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


#ifndef __QEXPANSION_H__
#define __QEXPANSION_H__

#include <Judy.h>

#include "mmisc.h"
#include "ttypes.h"

extern uint qexp_loaded;

void create_qexp(const char *fname, const Pvoid_t qexp);
uint load_qexp(const char *fname, uint ignore_notexist);
Pvoid_t qexp_merge(const Pvoid_t nexp);
Pvoid_t judify_qexp();
void close_qexp();
const glist *get_expansion(u32 xid);
glist *expand_all(const glist *lst);

#endif /* __QEXPANSION_H__ */
