/*
 *   preproc/tokenizer.h
 *   tokenizes a raw text stream
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

#ifndef __TOK_H__
#define __TOK_H__

#include <Judy.h>
#include <ixicon.h>
#include <ttypes.h>

extern u32              xid;
extern uint             toxicon_read_only;
extern uint             max_tok_len;
extern Pvoid_t          toxicon;

/* document buffers for tokenizer */
extern const char    *doc_buf;

/* for docdb */
extern struct obstack   tok_pos;
u32                     doc_pos;
//uint                    tok_cnt;

#endif /* __TOK_H__ */
