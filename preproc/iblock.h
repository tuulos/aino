/*
 *   preproc/encode_pos.h
 *   
 *   Copyright (C) 2006-2008 Ville H. Tuulos
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


#ifndef __IBLOCK_H__
#define __IBLOCK_H__

#define ENSURE_IBLOCK if (!iblock_left--) new_iblock();
#define ENSURE_IBLOCK_PRE(x) if (!iblock_left--){ x;
#define ENSURE_IBLOCK_POST(x) new_iblock(); x; }

extern FILE *toc_f;
extern FILE *data_f;
extern uint iblock_left;
extern uint iblock_cnt;
extern uint iblock_size;

void init_iblock(const char *sect_str);
void close_iblock();
void new_iblock();

#endif /* __IBLOCK_H__ */
