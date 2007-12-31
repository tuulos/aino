/*
 *   preproc/rawstream.h
 *   Handles untokenized document stream
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

#ifndef _RAWSTREAM_H_
#define _RAWSTREAM_H_

#include <serializer.h>
#include <ttypes.h>

struct rawdoc{
        const char *body;
        uint       body_size;
        char       *head;
};

struct rawdoc *pull_document();
void push_document(struct rawdoc *doc);

char *head_parse(const char **doc_buf);
char *head_get_field(const char *head, const char* field);
uint head_meta_match(const char *head, u32 first, u32 last);
Pvoid_t head_meta_bag(const char *head);
char *head_meta_add(char *head, u32 val);

#endif /* RAWSTREAM_H */

