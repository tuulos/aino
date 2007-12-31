/*
 *   lib/finnuio.h
 *   Some IO related operations
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

#ifndef _FINNU_IO_
#define _FINNU_IO_

#include <stdio.h>
#include <ttypes.h>
#include <mmisc.h>

/* Guarantees that all bytes are written */
int fio_write(int fd, const void *buf, uint len);

/* Guarantees that all bytes are read */
int fio_read(int fd, void *buf, uint len);

/* Copies src_sze bytes from src_fd to dest_fd from dest_offs onwards */
void fio_copy_file(int dest_fd, u64 dest_offs, int src_fd, u64 src_sze);

/* Truncates fd to sze bytes guaranteering that all bytes fit into the file */
int fio_truncate(int fd, u64 sze);

/* Reads a serialized glist from a FILE */
glist *fio_read_glist(FILE *in_f);

#endif /* _FINNU_IO_ */
