/*
 *   preproc/dehtml.h
 *   Dethtmlificator
 *   
 *   Copyright (C) 2005-2008 Ville H. Tuulos
 *
 *   Based on Kimmo Valtonen's groundbreaking work with HTML2txt.pm.
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

#ifndef __DEHTML_H__
#define __DEHTML_H__

#include <string.h>
#include <stdlib.h>

#define MAX_TITLE_LEN 1024

extern const char *doc_buf;
extern char       *out_buf;
extern char       title[];
extern uint       out_len;
extern uint       tag_matched;

#endif /* __DEHTML_H__ */
