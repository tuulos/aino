/*
 *   index/scorelib.h
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


#ifndef __SCORELIB_H__
#define __SCORELIB_H__

#include <gglist.h>
#include <ttypes.h>
#include <Judy.h>

#define RANKSET_SIZE 1000
 
struct scored_nfo{
        u32 xid;
        u32 hits;
        u32 freq;
};


struct scored_doc{
        u32 did;
        float score;
};

struct scored_ix{
        u32 xid;
        float score;
};


//Pvoid_t normalize_scores(Pvoid_t scores);
Pvoid_t layer_freq_high(Pvoid_t scores, uint layer, const glist *cueset);
//Pvoid_t layer_freq(Pvoid_t scores, u32 layer, const Pvoid_t cueset);
//Pvoid_t layer_ixscores(uint layer, const Pvoid_t cueset, const Pvoid_t prev);
//Pvoid_t score_part_some(uint part, const Pvoid_t segs, const Pvoid_t cueset);
//Pvoid_t score_part_all(uint part, const Pvoid_t cueset);
struct scored_ix *sort_scores(const Pvoid_t scores);
Pvoid_t expand_cueset(const glist *cues);

glist *find_hits(const glist *keys, uint did_mode);

Pvoid_t combine_layers(const Pvoid_t *layer_scores);

struct scored_doc *rank_dyn(const glist *hits, const Pvoid_t *layer_scores);
struct scored_doc *rank_brute(const glist *hits, const Pvoid_t *layer_scores);

char *compress_scores(const struct scored_ix *ixscores, uint ix_len,
        uint *buf_len, uint fac);

struct scored_ix *uncompress_scores(const char *buf, uint buf_len, 
        uint *ix_len);

float comp_error(const struct scored_ix *orig, 
        const struct scored_ix *est, uint len);

int ixscore_cmp(const void *i1, const void *i2);

struct scored_doc *top_ranked(const struct scored_doc *ranked, int len, int topk);

#endif /* __SCORELIB_H__ */
