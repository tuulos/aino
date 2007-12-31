/*
 *   lib/ttypes.h
 *   Typdefs
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

#ifndef __TYPES_H__
#define __TYPES_H__

#include <sys/types.h>
#include <stdint.h>

/* a generic 16-bit unsigned int */
typedef uint16_t u16;

/* a generic 32-bit unsigned int */
typedef uint32_t u32;

/* a generic 32-bit signed int */
typedef int32_t s32;

/* a generic 64-bit unsigned int */
typedef uint64_t u64;

/* document id */
//typedef uint32_t did_t;

/* ixeme id */
//typedef int32_t ixid_t;

//typedef unsigned uint;

#if 0
/* a generic type for containers etc. */
/* NB: This must be a 32-bit value, mostly due to
 * expand_bits() but maybe for other reasons also */
typedef union{
        u32    gen;
        did_t  did;
        ixid_t ixid;
} gen_t;
#endif

#endif /* __TYPES_H__ */
