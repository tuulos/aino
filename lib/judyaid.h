/*
 *   lib/judyaid.h
 *   Fixes some Judy's fallacies
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

#ifndef __JUDYAID_H__
#define __JUDYAID_H__

#include <Judy.h>
#include "ixemes.h"
#include "ttypes.h"

/* Judy doesn't continue automatically from the negative indices to the
 * positive ones. So we must help her over zero. However Judy seems to
 * happily continue from the positive indices to the negative but
 * screws up the values of negative indices. Thus we must stop as soon as
 * a negative value is found after the positives. */

/* NB: Type of idx is presumed to be compatible with ixid_t! */

#define JA_JLN(val, judy, idx){\
                ixid_t __prev_idx = (ixid_t)idx;\
                JLN(val, judy, idx);\
                if (__prev_idx < 0 && val == NULL){\
                        idx = 0;\
                        JLF(val, judy, idx);\
                }else if (__prev_idx > 0 && ((ixid_t)idx) < 0)\
                        val = NULL;\
        }

#define JA_JLF(val, judy, idx){\
                JLF(val, judy, idx);\
                if (val == NULL){\
                        idx = 0;\
                        JLF(val, judy, idx);\
                }\
        }

        

#endif /* __JUDYAID_H__ */
