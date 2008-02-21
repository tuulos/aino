/*
 *   ainopy/ainopy.c
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

#include <Python.h>

#include <dub.h>
#include <pparam.h>
#include <dexvm.h>
//#include <pcode.h>
#include <rrice.h>
#include <gglist.h>

#include <Judy.h>

static inline int check_sid(uint sid)
{
        if (sid >= DEX_NOF_SEG){
                PyErr_SetString(PyExc_RuntimeError, "Segment ID too big");
                return 1;
        }
        return 0;
}

static inline int check_did(uint did)
{
        if (did >= DEX_NOF_DOX){
                PyErr_SetString(PyExc_RuntimeError, "Document ID too big");
                return 1;
        }
        return 0;
}

static PyObject *ainopy_open(PyObject *self, PyObject *args)
{
        open_index();

        Py_INCREF(Py_None);
        return Py_None;
}

static PyObject *ainopy_inva(PyObject *self, PyObject *args)
{
        const inva_e *e = NULL;
        u32 offs = 0;
        u32 sid;
        int idx;
        u32 xid;

        if (!PyArg_ParseTuple(args, "I", &xid))
                return NULL;

        if ((idx = inva_find(xid, &e)) == -1){
                PyErr_SetString(PyExc_RuntimeError, "No such ixeme");
                return NULL;
        }

        PyObject* list = PyList_New(e->len);
        idx = 0;
        
        INVA_FOREACH_VAL(e, &offs, sid)
                PyObject *o = Py_BuildValue("I", sid);
                PyList_SET_ITEM(list, idx++, o);
        INVA_FOREACH_VAL_END

        return list;
}

static PyObject *ainopy_freq(PyObject *self, PyObject *args)
{
        const inva_e *e = NULL;
        u32 xid;
        int idx;

        if (!PyArg_ParseTuple(args, "I", &xid))
                return NULL;
        
        if ((idx = inva_find(xid, &e)) == -1){
                PyErr_SetString(PyExc_RuntimeError, "No such ixeme");
                return NULL;
        }

        return Py_BuildValue("i", e->len);
}

static PyObject *ainopy_xids(PyObject *self, PyObject *args)
{
        uint i;

        PyObject* list = PyList_New(DEX_NOF_IX);

        for (i = 0; i < DEX_NOF_IX; i++){
                PyObject *o = Py_BuildValue("i", 
                        dex_section[INVA].toc[i].val);
                PyList_SET_ITEM(list, i, o);
        }

        return list;
}

static PyObject *ainopy_fw(PyObject *self, PyObject *args)
{
        const void *p = NULL;
        u32 xid, sid, part, offs = 0;

        if (!PyArg_ParseTuple(args, "II", &part, &sid))
                return NULL;
       
       if (part >= NUM_FW_LAYERS){
                PyErr_SetString(PyExc_RuntimeError, "Layer ID too large");
                return NULL;
        }

        PyObject* list = PyList_New(0);

        p = fw_find(part, sid);
        if (!p)
                return list;

        DEX_FOREACH_VAL(p, &offs, xid)
                PyObject *o = Py_BuildValue("I", xid);
                PyList_Append(list, o);
                Py_DECREF(o);
        DEX_FOREACH_VAL_END

        return list;
}

static Pvoid_t xid_mapping(u32 sid)
{
        int tst, i, layer = NUM_FW_LAYERS;
        int xid_seen = 0;

        Pvoid_t mapping = NULL;
        while (layer--){
                const void *p = fw_find(layer, sid);
                if (!p)
                        continue; 

                u32 xid, offs = 0;

                DEX_FOREACH_VAL(p, &offs, xid)
                        J1S(tst, mapping, xid);
                DEX_FOREACH_VAL_END
        }
        return mapping;
}

static PyObject *ainopy_pos(PyObject *self, PyObject *args)
{
        u32 i, sid;
        
        if (!PyArg_ParseTuple(args, "I", &sid))
                return NULL;
        
        if (check_sid(sid))
                return NULL;
      
        PyObject* list = PyList_New(0);
        const u32 *seg = decode_segment(sid, dex_header.segment_size);
        if (!seg)
                return list; 

        for (i = 0; i < dex_header.segment_size && seg[i]; i++){
                PyObject *o = Py_BuildValue("I", seg[i]);
                PyList_Append(list, o);
                Py_DECREF(o);
        }
        return list;
}

static PyObject *segments_in_doc(u32 did)
{

        PyObject* list = PyList_New(0);
        u32 sid;

        for (sid = DID2SID(did); sid < DEX_NOF_SEG; sid++){
                if (SID2DID(sid) != did)
                        break;
                PyObject *o = Py_BuildValue("i", sid);
                PyList_Append(list, o);
                Py_DECREF(o);
        }

        return list;
}

static PyObject *ainopy_did2info(PyObject *self, PyObject *args)
{
        u32 did;

	if (!PyArg_ParseTuple(args, "I", &did))
                return NULL;
        
        if (check_did(did))
                return NULL;

	const doc_e *nfo = info_find(did);

        return Py_BuildValue("if", nfo->key, nfo->prior);
}

static PyObject *ainopy_sid2doc(PyObject *self, PyObject *args)
{
        u32 sid;
        u32 did;

        if (!PyArg_ParseTuple(args, "I", &sid))
                return NULL;

        if (check_sid(sid))
                return NULL;

        did = SID2DID(sid);
        PyObject* list = segments_in_doc(did);

        PyObject *ret = Py_BuildValue("(iO)", did, list);
        Py_DECREF(list);
        return ret;
}

static PyObject *ainopy_sid2did(PyObject *self, PyObject *args)
{
        u32 sid;
        u32 did;

        if (!PyArg_ParseTuple(args, "I", &sid))
                return NULL;

        if (check_sid(sid))
                return NULL;

        return Py_BuildValue("i", SID2DID(sid));
}

static PyObject *ainopy_did2doc(PyObject *self, PyObject *args)
{
        u32 did;

        if (!PyArg_ParseTuple(args, "I", &did))
                return NULL;
        
        if (check_did(did))
                return NULL;

        return segments_in_doc(did);
}

static PyObject *ainopy_did2key(PyObject *self, PyObject *args)
{
        u32 did;
        
        if (!PyArg_ParseTuple(args, "I", &did))
                return NULL;
        
        if (check_did(did))
                return NULL;

        return Py_BuildValue("i", DID2KEY(did));
}

static PyObject *ainopy_info(PyObject *self, PyObject *args)
{
        return Py_BuildValue("{s:i, s:i, s:i, s:i, s:i}",
                        "Index ID", dex_header.index_id,
                        "Index block", dex_header.iblock,
                        "Number of documents", DEX_NOF_DOX,
                        "Number of segments", DEX_NOF_SEG,
                        "Number of ixemes", DEX_NOF_IX);
}

static PyObject *ainopy_stats(PyObject *self, PyObject *args)
{
        PyObject *d = PyDict_New();
        uint i;

        for (i = 0; i < DEX_NOF_SECT; i++){
                PyObject *o = Py_BuildValue("(ii)", dex_section[i].fetches,
                                dex_section[i].fetch_faults);

                PyDict_SetItemString(d, dex_names[i], o);
                Py_DECREF(o);
        }

        return d;
}

        
static PyMethodDef ainopy_methods[] = {
        
        {"open", ainopy_open, METH_NOARGS,
                "Opens index specified by env.vars NAME and IBLOCK"},
        
        {"inva", ainopy_inva, METH_VARARGS,
                "Returns the inverted list of the given ixeme ID"},
        
        {"freq", ainopy_freq, METH_VARARGS,
                "Returns the frequence of the given ixeme ID"},
        
        {"xids", ainopy_xids, METH_NOARGS,
                "Lists all ixeme IDs in the index"},

        {"fw", ainopy_fw, METH_VARARGS,
                "Returns the forward index entry of the given segment ID"},
        
        {"pos", ainopy_pos, METH_VARARGS,
                "Returns the token positions for the given document ID"},
       
        {"sid2doc", ainopy_sid2doc, METH_VARARGS,
                "Given a segment ID, returns tuple (did, [sid0,..,sidN]) "
                "corresponding to the document to which the segment ID "
                "belongs to"},

        {"did2doc", ainopy_did2doc, METH_VARARGS,
                "Lists segments belonging to the given document ID"},
        
        {"did2key", ainopy_did2key, METH_VARARGS,
                "Returns the key corresponding to the given document ID"},
	
	{"sid2did", ainopy_sid2did, METH_VARARGS,
                "Segment ID to document ID"},
        
	{"docinfo", ainopy_did2info, METH_VARARGS,
                "Returns document information"},

        {"info", ainopy_info, METH_NOARGS,
                "Index information"},
        
        {"stats", ainopy_stats, METH_NOARGS,
                "Index usage statistics"},
        
        {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC initainopy()
{
        Py_InitModule("ainopy", ainopy_methods);

        dub_init();
}
