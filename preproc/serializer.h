/*
 *   preproc/serializer.h
 *   serializes tokenized stream
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

#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#include <Judy.h>

#include <ttypes.h>
#include <dub.h>
#include <gglist.h>

void open_serializer();
void serialize_head(u32 id, const Pvoid_t head_bag);
void serialize_segment(const Pvoid_t segment_bag);
const glist *pull_head(u32 *key);
const glist *pull_segment();
inline void push_head(u32 key, const glist *g);
inline void push_segment(const glist *g);

#endif /* __SERIALIZER_H__ */
