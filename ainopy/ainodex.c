/*
 *   ainopy/ainodex.c
 *   
 *   Copyright (C) 2007-2008 Ville H. Tuulos
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

#include <Python.h>
#include <Judy.h>

#include <qexpansion.h>
#include <ixicon.h>
#include <pparam.h>
#include <dexvm.h>
#include <dub.h>
#include <mmisc.h>
#include <rrice.h>
#include <scorelib.h>

#define QEXP 0

#define MIN_SCORE 0.00001

typedef struct{
        u32 layer;
        int normalized;
        Pvoid_t scores;
} layer_e;
        
typedef struct{
        u32 xid;
        u32 freq;
} score_e;

static void destroy_judyL(void *j)
{
        int tst;
        JLFA(tst, j);
}

static void destroy_judy1(void *j)
{
        int tst;
        J1FA(tst, j);
}

static void destroy_generic(void *j)
{
        free(j);
}

static void destroy_layer(void *j)
{
        layer_e *layer = (layer_e*)j;
        int tst;
        JLFA(tst, layer->scores);
        free(layer);
}

glist *sequence_to_glist(PyObject *lst)
{
        int i, len = PySequence_Length(lst);
        glist *g = malloc(sizeof(glist) + len * 4);
        g->len = len;

        for (i = 0; i < len; i++){
                PyObject *errtype = NULL;
                PyObject *e = PySequence_GetItem(lst, i);
                if (!e){
                        free(g);
                        return NULL;
                }
                long val = PyInt_AsLong(e);
                if ((errtype = PyErr_Occurred())){
                        free(g);
                        PyErr_SetString(errtype, "Item not a valid ID");
                        return NULL;
                }
                g->lst[i] = (u32)val;  
        }
        return g;
}

const Pvoid_t *unwrap_layer_list(PyObject *layer_scores_o, int normalized)
{
        static Pvoid_t layer_scores[NUM_FW_LAYERS];
        memset(layer_scores, 0, NUM_FW_LAYERS * sizeof(void*));
        int i, len = PySequence_Length(layer_scores_o);

        for (i = 0; i < len; i++){
                PyObject *o = PySequence_Fast_GET_ITEM(layer_scores_o, i);
                if (!PyCObject_Check(o))
                        continue;
                
		layer_e *layer = PyCObject_AsVoidPtr(o);
                if (layer_scores[layer->layer]){
                        PyErr_SetString(PyExc_RuntimeError, 
                                "Duplicate layers");
			return NULL;
                }
                if (layer->normalized < normalized){
                        PyErr_SetString(PyExc_RuntimeError,
                                "Layer not normalized");
                        return NULL;
                }
                layer_scores[layer->layer] = layer->scores;
        }
	return layer_scores;
}	

static PyObject *ainodex_open(PyObject *self, PyObject *args)
{
        uint do_qexp;

        dub_init();
        open_index();
        const char *fname = pparm_common_name("ixi");
        load_ixicon(fname);

        PPARM_INT(do_qexp, QEXP);
        if (do_qexp){
                char *fname = pparm_common_name("qexp");
                load_qexp(fname, 0);
                free(fname);
        }

        Py_INCREF(Py_None);
        return Py_None;
}

static PyObject *ainodex_ixicon_entry(PyObject *self, PyObject *args)
{
	PyObject *rixicon_o = NULL;
	u32 xid = 0;

        if (!PyArg_ParseTuple(args, "OI", &rixicon_o, &xid))
                return NULL;

        if (!PyCObject_Check(rixicon_o)){
                PyErr_SetString(PyExc_TypeError, "Argument not a reverse ixicon");
                return NULL;
        }
        
	const Pvoid_t rixi = PyCObject_AsVoidPtr(rixicon_o); 
	Word_t *ptr;
	JLG(ptr, rixi, xid);
	if (ptr)
        	return Py_BuildValue("s", (char*)*ptr);
	else
        	Py_INCREF(Py_None);
	        return Py_None;
}

static PyObject *ainodex_ixicon(PyObject *self, PyObject *args)
{
	int make_dict = 1;

        if (!PyArg_ParseTuple(args, "|i", &make_dict))
                return NULL;

        Pvoid_t ixi = reverse_ixicon();
        if (!make_dict)
		return PyCObject_FromVoidPtr(ixi, destroy_judyL);

        Word_t xid = 0;
        Word_t *ptr;

        PyObject* dict = PyDict_New();

        JLF(ptr, ixi, xid);
        while (ptr){
                char *tok = (char*)*ptr;

                PyObject *key = Py_BuildValue("I", xid);
                PyObject *val = PyBuffer_FromMemory(tok, strlen(tok));

                if (PyDict_SetItem(dict, key, val)){
                        Py_DECREF(key);
                        Py_DECREF(val);
                        Py_DECREF(dict);
                        PyErr_SetString(PyExc_RuntimeError,
                                "Could not build ixicon");
                        JLFA(xid, ixi);
                        return NULL;
                }

                Py_DECREF(key);
                Py_DECREF(val);

                JLN(ptr, ixi, xid);
        }

        JLFA(xid, ixi);
        return dict;
}



static PyObject *ainodex_token2ixeme(PyObject *self, PyObject *args)
{
        const char *token;

        if (!PyArg_ParseTuple(args, "s", &token))
                return NULL;
        
        return Py_BuildValue("i", get_xid(token));
}

static PyObject *scorepy_ixeme2token(PyObject *self, PyObject *args)
{
        uint xid;

        if (!PyArg_ParseTuple(args, "I", &xid))
                return NULL;

        return Py_BuildValue("s", get_ixeme(xid));
}

static PyObject *ainodex_normtable(PyObject *self, PyObject *args)
{
        const inva_e *e = NULL;
        int i;

        score_e *r = xmalloc(DEX_NOF_IX * sizeof(score_e));

        for (i = 0; i < DEX_NOF_IX; i++){
                e = (const inva_e*)fetch_item(INVA, i);
                r[i].xid = dex_section[INVA].toc[i].val;
                r[i].freq = e->len;
        }

        PyObject *ret = Py_BuildValue("s#", r, DEX_NOF_IX * sizeof(score_e));
        free(r);        
        return ret;
}

static PyObject *ainodex_findhits(PyObject *self, PyObject *args)
{
        PyObject *key_o;
        int did_mode;

        if (!PyArg_ParseTuple(args, "OI", &key_o, &did_mode))
                return NULL;

        if (!PySequence_Check(key_o)){
                PyErr_SetString(PyExc_TypeError, "Expected a sequence");
                return NULL;
        }

        glist *keys = sequence_to_glist(key_o);
        if (!keys)
                return NULL;
        
        glist *hits = find_hits(keys, did_mode);
        //return PyCObject_FromVoidPtr(hits, destroy_generic);
        return Py_BuildValue("IN", hits->len, 
                PyCObject_FromVoidPtr(hits, destroy_generic));
}

static PyObject *ainodex_list_to_hits(PyObject *self, PyObject *args)
{
	PyObject *lst;
	if (!PyArg_ParseTuple(args, "O", &lst))
		return NULL;
		
	if (!PySequence_Check(lst)){
		PyErr_SetString(PyExc_TypeError, "Expected a sequence");
		return NULL;
	}
        
	glist *hits = sequence_to_glist(lst);
        return PyCObject_FromVoidPtr(hits, destroy_generic);
}

static PyObject *ainodex_hit_contents(PyObject *self, PyObject *args)
{
        PyObject *hits_o;

        if (!PyArg_ParseTuple(args, "O", &hits_o))
                return NULL;

        if (!PyCObject_Check(hits_o)){
                PyErr_SetString(PyExc_TypeError, "Item not a hits object");
                return NULL;
        }
        
        const glist *hits = PyCObject_AsVoidPtr(hits_o); 

        PyObject *ret = PyList_New(hits->len);
        int i;
        for (i = 0; i < hits->len; i++){
                PyObject *obj = Py_BuildValue("I", hits->lst[i]);
                PyList_SET_ITEM(ret, i, obj);
        }
        return ret;
}

static PyObject *ainodex_expand_cueset(PyObject *self, PyObject *args)
{
        PyObject *cues_o;
        Word_t len;

        if (!PyArg_ParseTuple(args, "O", &cues_o))
                return NULL;
                
        glist *cues = sequence_to_glist(cues_o);
        if (!cues)
                return NULL;

        Pvoid_t cueset = expand_cueset(cues);
        J1C(len, cueset, 0, -1);

        free(cues);
        return Py_BuildValue("IN", len, 
                PyCObject_FromVoidPtr(cueset, destroy_judy1));
}



static PyObject *ainodex_new_layer(PyObject *self, PyObject *args)
{
        PyObject *scores_o = NULL;
        layer_e *layer = NULL;
        PyObject *cues_o;
        int lay;
        
        if (!PyArg_ParseTuple(args, "iO|O", &lay, &cues_o, &scores_o))
                return NULL;

        if (!PyCObject_Check(cues_o)){
                PyErr_SetString(PyExc_TypeError, "2 argument not a cueset");
                return NULL;
        }
        
        if (scores_o){
                if (!PyCObject_Check(scores_o)){
                        PyErr_SetString(PyExc_TypeError, 
                                "3 argument not a score object");
                        return NULL;
                }
                layer = PyCObject_AsVoidPtr(scores_o);
        }else{
                layer = xmalloc(sizeof(layer_e));
                layer->scores = NULL;
        }
       
        layer->normalized = 0;
        layer->layer = lay;
        
        const Pvoid_t cueset = PyCObject_AsVoidPtr(cues_o);
        //if (lay < 0)
        //        layer->scores = score_part_some(lay, cueset, cueset);
        //else
        layer->scores = layer_freq_high(layer->scores, lay, cueset);
        
        //Word_t dd;
        //JLC(dd, layer->scores, 0, -1);
        //dub_msg("%u LAYER %u", lay, dd);
        
        if (scores_o)
                return scores_o;
        else
                return PyCObject_FromVoidPtr(layer, destroy_layer);
}

static PyObject *ainodex_serialize_layer(PyObject *self, PyObject *args)
{
        static char *buf;
        static uint buf_len;

        PyObject *scores_o = NULL;

        if (!PyArg_ParseTuple(args, "O", &scores_o))
                return NULL;

        if (!PyCObject_Check(scores_o)){
                PyErr_SetString(PyExc_TypeError, 
                        "Argument not a layer object");
                return NULL;
        }

        const layer_e *layer = PyCObject_AsVoidPtr(scores_o);
        
        /*
        if (layer->normalized){
                PyErr_SetString(PyExc_TypeError, 
                        "Can't serialize a normalized layer");
                return NULL;
        }
        */

        Word_t prev, no, xid;
        Word_t *ptr;
        u32 offs;

        JLC(no, layer->scores, 0, -1);
        if (no >= buf_len){
                buf_len = no;
                free(buf);
                buf = xmalloc(no * 16 + 16);
                memset(buf, 0, no * 16 + 16);
        }

        prev = xid = offs = 0;
        
        elias_gamma_write(buf, &offs, layer->layer + 1);
        elias_gamma_write(buf, &offs, layer->normalized + 1);
        elias_gamma_write(buf, &offs, no);

        JLF(ptr, layer->scores, xid);
        while (ptr){
                Word_t val = xid - prev;
                prev = xid;
                elias_gamma_write(buf, &offs, val);
                if (layer->normalized){
                        u32 val = *(float*)ptr * (1.0 / MIN_SCORE);
                        if (!val)
                                ++val;
                        elias_gamma_write(buf, &offs, val);
                }else
                        elias_gamma_write(buf, &offs, *ptr);
                        
                JLN(ptr, layer->scores, xid);
        }

        offs = BITS_TO_BYTES(offs);
        PyObject *ret = PyString_FromStringAndSize(buf, offs);
        memset(buf, 0, offs);
        return ret;
}

static PyObject *ainodex_deserialize_layer(PyObject *self, PyObject *args)
{
        const char *buf;
        uint buf_len, offs = 0;
        Word_t *ptr;
        Word_t xid = 0;
        PyObject *layer_lst = NULL;
        PyObject *layer_o = NULL;
        layer_e *layer = NULL;

        if (!PyArg_ParseTuple(args, "s#|O", &buf, &buf_len, &layer_lst))
                return NULL;
        
        int layer_id = elias_gamma_read(buf, &offs) - 1;
        int layer_normalized = elias_gamma_read(buf, &offs) - 1;
        
        if (layer_lst){
                if (!PySequence_Check(layer_lst)){
                        PyErr_SetString(PyExc_TypeError, 
                                "Argument not a layer sequence");
                        return NULL;
                }
                
                PyObject *e = PySequence_GetItem(layer_lst, layer_id);
                if (!e){
                        PyErr_SetString(PyExc_TypeError, 
                                "Layer not found in the list");
                        return NULL;
                }

                if (PyCObject_Check(e)){
                        layer_o = e;
                        layer = PyCObject_AsVoidPtr(e);
                        if (layer->layer != layer_id){
                                PyErr_SetString(PyExc_TypeError, 
                                        "Layer IDs don't agree");
                                return NULL;
                        }
                        if (layer->normalized != layer_normalized){
                                PyErr_SetString(PyExc_TypeError, 
                                        "Normalization doesn't match");
                                return NULL;
                        }
                }else
                        Py_DECREF(e);
        }
        if (!layer){
                layer = xmalloc(sizeof(layer_e));
                layer->normalized = layer_normalized;
                layer->scores = NULL;
                layer->layer = layer_id;
        }
        
        u32 no = elias_gamma_read(buf, &offs);
        while (no--){
                xid += elias_gamma_read(buf, &offs);
                JLI(ptr, layer->scores, xid);
                if (layer_normalized){
                        u32 val = elias_gamma_read(buf, &offs);
                        float sco = *(float*)ptr + val * MIN_SCORE;
                        memcpy(ptr, &sco, sizeof(float));
                }else
                        *ptr += elias_gamma_read(buf, &offs);
        }

        offs = BITS_TO_BYTES(offs);
        if (!layer_o){
                layer_o = PyCObject_FromVoidPtr(layer, destroy_layer);
                if (layer_lst)
                        PySequence_SetItem(layer_lst, layer_id, layer_o);
        }
        PyObject *ret = Py_BuildValue("IIN", offs, layer->layer, layer_o);
        return ret;
}

static PyObject *ainodex_top_ixemes(PyObject *self, PyObject *args)
{
        PyObject *layer_scores_o = NULL;
	int num = 10;

        if (!PyArg_ParseTuple(args, "O|i", &layer_scores_o, &num))
                return NULL;

	const Pvoid_t *layer_scores = unwrap_layer_list(layer_scores_o, 0);
	if (!layer_scores)
		return NULL;
	Pvoid_t all_scores = combine_layers(layer_scores);
	struct scored_ix *sorted = sort_scores(all_scores);	

        PyObject *lst = PyList_New(num);
	int i;
	for (i = 0; i < num; i++)
                PyList_SET_ITEM(lst, i, Py_BuildValue("If",
			sorted[i].xid, sorted[i].score));

	JLFA(i, all_scores);
	free(sorted);
	return lst;
}


static PyObject *ainodex_sync_layers(PyObject *self, PyObject *args)
{
        PyObject *layer_scores_o = NULL;

        if (!PyArg_ParseTuple(args, "O", &layer_scores_o))
                return NULL;
        
	if (!PySequence_Check(layer_scores_o)){
                PyErr_SetString(PyExc_TypeError, "argument not a sequence");
		return NULL;
        }
        
        int i, len = PySequence_Length(layer_scores_o);
	const Pvoid_t *layer_scores = unwrap_layer_list(layer_scores_o, 0);
	if (!layer_scores)
		return NULL;
	Pvoid_t all_scores = combine_layers(layer_scores);
	
	Word_t xid = 0;
	Word_t *ptr1, *ptr2;

	for (i = 0; i < len; i++){
		xid = 0;
		JLF(ptr1, layer_scores[i], xid);
		while (ptr1){
			JLG(ptr2, all_scores, xid);
			*ptr1 = *ptr2;
			JLN(ptr1, layer_scores[i], xid);
		}
        }

	JLFA(i, all_scores);
        Py_INCREF(Py_None);
        return Py_None;
}

static PyObject *ainodex_layer_contents(PyObject *self, PyObject *args)
{
        PyObject *scores_o = NULL;

        if (!PyArg_ParseTuple(args, "O", &scores_o))
                return NULL;

        if (!PyCObject_Check(scores_o)){
                PyErr_SetString(PyExc_TypeError, 
                        "Argument not a layer object");
                return NULL;
        }

        const layer_e *layer = PyCObject_AsVoidPtr(scores_o);
        
        Word_t *ptr;
        int i = 0;
        Word_t num_sco, xid = 0;
        JLC(num_sco, layer->scores, 0, -1);
        PyObject *lst = PyList_New(num_sco);
        
        JLF(ptr, layer->scores, xid);
        while (ptr){
                PyObject *e;
                if (layer->normalized)
                        e = Py_BuildValue("If", xid, *(float*)ptr);
                else
                        e = Py_BuildValue("II", xid, *ptr);
                
                PyList_SET_ITEM(lst, i++, e);
                JLN(ptr, layer->scores, xid);
        }
        return Py_BuildValue("IIN", layer->layer, layer->normalized, lst);
}

static PyObject *ainodex_list_score(PyObject *self, PyObject *args)
{
        PyObject *scores_o = NULL;
        PyObject *list = NULL;

        if (!PyArg_ParseTuple(args, "OO", &list, &scores_o))
                return NULL;
        
        if (!PyList_Check(list)){
                PyErr_SetString(PyExc_TypeError, "1 argument not a list");
                return NULL;
        }

        if (!PyCObject_Check(scores_o)){
                PyErr_SetString(PyExc_TypeError, 
                        "2 Argument not a layer object");
                return NULL;
        }

        const layer_e *layer = PyCObject_AsVoidPtr(scores_o);
        if (!layer->normalized){
                PyErr_SetString(PyExc_RuntimeError,
                        "Layer not normalized");
                return NULL;
        }

        int i, len = PyList_Size(list);
        float sco = 0;

        for (i = 0; i < len; i++){
                PyObject *o = PyList_GET_ITEM(list, i);
                if (!PyInt_Check(o)){
                        PyErr_SetString(PyExc_TypeError, 
                                "List item not an integer");
                        return NULL;
                }
                long xid = PyInt_AsLong(o);
                Word_t *ptr;
                JLG(ptr, layer->scores, xid);
                if (ptr)
                        sco += *(float*)ptr;
        }
        return Py_BuildValue("f", sco);
}

static PyObject *ainodex_normtable_to_judy(PyObject *self, PyObject *args)
{

        Pvoid_t norm = NULL;
        const char *buf;
        uint buf_len;

        if (!PyArg_ParseTuple(args, "s#", &buf, &buf_len))
                return NULL;

        const score_e *e = (const score_e*)buf;
        u32 no = buf_len / sizeof(score_e);
        while (no--){
                Word_t *ptr;
                JLI(ptr, norm, e[no].xid);
                *ptr += e[no].freq;
        }
        
        return PyCObject_FromVoidPtr(norm, destroy_judyL);
}

static PyObject *ainodex_normalize_layer(PyObject *self, PyObject *args)
{
        PyObject *scores_o;
        PyObject *norm_o;
        PyObject *cueset_o = NULL;
        Word_t cueset_len = 0;

        if (!PyArg_ParseTuple(args, "OO|O", &scores_o, &norm_o, &cueset_o))
                return NULL;

        if (!PyCObject_Check(scores_o)){
                PyErr_SetString(PyExc_TypeError, 
                        "1 Argument not a score object");
                return NULL;
        }

        if (!PyCObject_Check(norm_o)){
                PyErr_SetString(PyExc_TypeError, 
                        "2 Argument not a normalization object");
                return NULL;
        }
        
        if (cueset_o){
                if (PyCObject_Check(cueset_o)){
                        const Pvoid_t cueset = PyCObject_AsVoidPtr(cueset_o);
                        J1C(cueset_len, cueset, 0, -1);

                }else if (PyInt_Check(cueset_o)){
                        cueset_len = PyInt_AsLong(cueset_o);
                }else{
                        PyErr_SetString(PyExc_TypeError, 
                                "3 Argument not a cueset object or "
                                "cueset length");
                        return NULL;
                }
                dub_msg("CUESET_LEN %u", cueset_len);
        }

        layer_e *layer = PyCObject_AsVoidPtr(scores_o);
        const Pvoid_t norm = PyCObject_AsVoidPtr(norm_o);

        Word_t xid = 0;
        Word_t *ptr;
        Word_t *ptr1;
        JLF(ptr, layer->scores, xid);
        while (ptr){
                float sco = *ptr;
                JLG(ptr1, norm, xid);
                if (ptr1){
                        /* Ixeme scoring formula */
                        if (cueset_len)
                                /* Jaccard coefficent */
                                sco /= *ptr1 + cueset_len - *ptr;
                        else
                                /* P(q|w) */
                                sco /= *ptr1;
                }else
                        sco = 0;
                        /*
                        PyErr_SetString(PyExc_ValueError, 
                                "Xid missing in normtable");
                        return NULL;
                        */

                /* sco may be > 1.0 if the normalization vector and
                 * layer frequencies are not in sync */
                if (sco > 1.0){
                        dub_warn("Score %f > 1.0! Set to 1.0.", sco);
                        sco = 1.0;
                }
                memcpy(ptr, &sco, sizeof(float));
                JLN(ptr, layer->scores, xid);
        }

        layer->normalized = 1;
        Py_INCREF(Py_None);
        return Py_None;
}

static PyObject *ainodex_rank(PyObject *self, PyObject *args)
{
        PyObject *hits_o, *layer_scores_o;
        int brute_cutoff = 1500;
        int results_as_list = 0;
        struct scored_doc *ranked;
        PyObject *ret = NULL;


        if (!PyArg_ParseTuple(args, "OO|ii", &hits_o, &layer_scores_o, 
                        &results_as_list, &brute_cutoff))
                return NULL;
       
        if (!PyCObject_Check(hits_o)){
                PyErr_SetString(PyExc_TypeError, "Item not a hits object");
		return NULL;
        }

        if (!PySequence_Check(layer_scores_o)){
                PyErr_SetString(PyExc_TypeError, "2 argument not a sequence");
		return NULL;
        }
        const glist *hits = PyCObject_AsVoidPtr(hits_o); 
        const Pvoid_t *layer_scores = unwrap_layer_list(layer_scores_o, 1);
	if (!layer_scores)
		return NULL;

        if (hits->len < brute_cutoff)
                ranked = rank_brute(hits, layer_scores);
        else
                ranked = rank_dyn(hits, layer_scores);

        int i, len = 0;
        while (ranked[len].did || ranked[len].score){
                ranked[len].did = DID2KEY(ranked[len].did);
                ++len;
        }

        if (results_as_list){
                ret = PyList_New(len);
                for (i = 0; i < len; i++){
                        PyObject *obj = Py_BuildValue("fI", 
                                ranked[i].score, ranked[i].did);
                        PyList_SET_ITEM(ret, i, obj);
                }
        }else
                ret = PyString_FromStringAndSize((const char*)ranked, 
                        len * sizeof(struct scored_doc));
        free(ranked);
        return ret;
}

static PyObject *ainodex_merge_ranked(PyObject *self, PyObject *args)
{
        const char *buf;
        uint buf_len;
        int topk = RANKSET_SIZE;

        if (!PyArg_ParseTuple(args, "s#|i", &buf, &buf_len, &topk))
                return NULL;

        const struct scored_doc *r = (const struct scored_doc*)buf;
        int len = buf_len / sizeof(struct scored_doc);

        struct scored_doc *top = top_ranked(r, len, topk);
        PyObject *ret = Py_BuildValue("s#", top, 
                (len > topk ? topk: len) * sizeof(struct scored_doc));
        free(top);        
        return ret;
}

static PyMethodDef ainodex_methods[] = {
        {"open", ainodex_open, METH_VARARGS,
                "Opens index specified by env.vars NAME and IBLOCK"},
        {"ixicon", ainodex_ixicon, METH_VARARGS,
                "Return a mapping from ixeme IDs to tokens"},
        {"ixicon_entry", ainodex_ixicon_entry, METH_VARARGS,
                "Return an entry from reverse ixicon"},
        {"token2ixeme", ainodex_token2ixeme, METH_VARARGS,
                "Return ixeme ID of the given token"},
        {"ixeme2token", scorepy_ixeme2token, METH_VARARGS,
                "Return the token corresponding to the given ixeme ID (slow!)"},
        {"hits", ainodex_findhits, METH_VARARGS,
                "Find documents matching the given ixemes"},
        {"hit_contents", ainodex_hit_contents, METH_VARARGS,
                "Converts a hit object to list"},
        {"list_to_hits", ainodex_list_to_hits, METH_VARARGS,
                "Converts a list to hit object"},
        {"expand_cueset", ainodex_expand_cueset, METH_VARARGS,
                "Makes a cueset given a list of cue ixemes"},
        {"new_layer", ainodex_new_layer, METH_VARARGS,
                "Cueset frequency in the given layer"},
        {"serialize_layer", ainodex_serialize_layer, METH_VARARGS,
                "Writes a layer object to string"},
        {"deserialize_layer", ainodex_deserialize_layer, METH_VARARGS,
                "Reads a layer object from string"},
        {"top_ixemes", ainodex_top_ixemes, METH_VARARGS,
                "Return the highest scoring ixemes from the layers"},
        {"sync_layers", ainodex_sync_layers, METH_VARARGS,
                "Make sure that ixeme counts match on all layers"},
        {"layer_contents", ainodex_layer_contents, METH_VARARGS,
                "Returns the layer contents"},
        {"score_list", ainodex_list_score, METH_VARARGS,
                "Compute score for the given list of XIDs"},
        {"normtable_to_judy", ainodex_normtable_to_judy, METH_VARARGS,
                "Converts an array of ixeme frequencies to Judy"},
        {"normtable", ainodex_normtable, METH_NOARGS,
                "Ixeme frequencies"},
        {"normalize_layer", ainodex_normalize_layer, METH_VARARGS,
                "Normalizes counts"},
        {"rank", ainodex_rank, METH_VARARGS,
                "Ranks a hitset"},
        {"merge_ranked", ainodex_merge_ranked, METH_VARARGS,
                "Merges lists of ranked documents"},
        {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initainodex()
{
        Py_InitModule("ainodex", ainodex_methods);
}
