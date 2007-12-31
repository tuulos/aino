/*
 *   preproc/docdb_builder.h
 *   builds the document database
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


#ifndef __DOCDB_BUILDER_H__
#define __DOCDB_BUILDER_H__

#include <dub.h>
#include <obstack.h>

#define obstack_chunk_alloc xmalloc0
#define obstack_chunk_free free

void docdb_init();
void docdb_insert(struct obstack *tok_pos, u32 key, char *head, 
                  const char *doc_buf);
void docdb_finish();

#endif /* __DOCDB_BUILDER_H__ */
