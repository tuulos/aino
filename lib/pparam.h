/*
 *   lib/pparam.h
 *   Parameters via environment
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

#ifndef __PPARAM_H__
#define __PPARAM_H__

#include <stdlib.h>
#include <errno.h>
#include <dub.h>

#define LINEMAX 2048

#ifdef DEBUG
#define DO_PRINT
#else
#define DO_PRINT if (dub_verbose)
#endif

char *pparm_common_name(const char *postfix);
void pparm_read_env(const char *fname);

#define PPARM_STR_D(var, name) { if(getenv(#name)) var = getenv(#name); else\
                                 dub_die(#name" not specified"); \
                                 DO_PRINT dub_dbg("Parameter "#name\
                                                 "=\"%s\"", var); }

#define PPARM_STR(var, name) { var = (getenv(#name) ? getenv(#name): name);\
                                DO_PRINT dub_dbg("Parameter "#name\
                                                "=\"%s\"", var); }

#define PPARM_FLOAT(var, name) { if (getenv(#name)){\
                                errno = 0;\
                                var = (float)strtod(getenv(#name), NULL);\
                                if (errno)\
                                        dub_die(\
                                        "Parameter "#name"=%s out of range",\
                                                getenv(#name));\
                                }else{\
                                        var = name;\
                                }\
                                DO_PRINT dub_dbg("Parameter "#name"=%f", var); }

#define PPARM_INT(var, name) { if (getenv(#name)){\
                                errno = 0;\
                                var = strtoul(getenv(#name), NULL, 10);\
                                if (errno)\
                                        dub_die(\
                                        "Parameter "#name"=%s out of range",\
                                                getenv(#name));\
                                }else{\
                                        var = name;\
                                }\
                                DO_PRINT dub_dbg("Parameter "#name"=%u", var); }

#define PPARM_INT_D(var, name) { if (getenv(#name)){\
                                errno = 0;\
                                var = strtoul(getenv(#name), NULL, 10);\
                                if (errno)\
                                        dub_die(\
                                        "Parameter "#name"=%s out of range",\
                                                getenv(#name));\
                                }else{\
                                        dub_die(#name" not specified");\
                                }\
                                DO_PRINT dub_dbg("Parameter "#name"=%u", var); }



#endif /* __PPARAM_H__ */
