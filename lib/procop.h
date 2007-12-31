/*
 *   lib/procop.h
 *   Process control
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


#ifndef __PROCOP_H__
#define __PROCOP_H__

#include <sys/types.h>

int exec_and_read(const char *proc, int *proc_stdin, pid_t *child, 
                  char *const envp[]);
int exec_and_write(const char *proc, int *proc_stdout, pid_t *child,
                  char *const envp[]);
int exec_and_rw(const char *proc, int *proc_stdout, pid_t *child,
                char *const envp[]);

#endif /* __PROCOP_H__ */
