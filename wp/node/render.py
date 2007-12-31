
import re, time, os, sys, cStringIO, traceback, array, zlib
import ainodex, erlay, make_query
from web import template, net

from netstring import *

SNIP_MAX_LEN = 500

cue_re = re.compile("/\w+|/\".+?\"")
nice_re = re.compile("[^a-zA-Z0-9\" ]")

cand_re = re.compile("[\.\*\n] [A-Z][\'\|\[\]a-zA-Z0-9\(\), ]*?[\.\n]")
results_tmpl = template.Template(file("results.tmpl").read(),
        filter = net.websafe)
template.Template.globals['range'] = range
template.Template.globals['max'] = max
template.Template.globals['min'] = min

def read_docs():
        global doc_db, doc_idx
        doc_db = file("%s.docs" % os.environ['NAME'])
        doc_idx = {}
        for l in file("%s.docs.index" % os.environ['NAME']):
                key, offs, size = map(int, l.split())
                doc_idx[key] = (offs, size)

def make_snippet(dockey, query, layers):
        offs, size = doc_idx[dockey]
        doc_db.seek(offs)
        title, doc = zlib.decompress(doc_db.read(size)).split('\0')
        keys = [filter(lambda c: c.isalnum(), key.lower())
                for key in query.split()]
        
        unic = re.sub("\\.*?\]", "", doc.decode('iso-8859-1'))
        unic = re.sub("[\[\]\|]", "", unic)
        candidates = re.findall(cand_re, unic)
        scores = []
        
        for sentence in candidates:
                txt = re.sub("[^a-zA-Z0-9 ]", " ", sentence.lower())
                xidbag = filter(lambda x: x,
                        map(ainodex.token2ixeme, txt.split()))
                s = sum(1.0 for key in keys if re.search("\W%s\W" % key, txt))
                s += sum(ainodex.score_list(xidbag, layer)
                        for layer in layers if layer)
                scores.append((s, sentence))
        scores.sort(reverse = True)
        
        snip = ""
        for score, sentence in scores:
                if len(snip) + len(snip) > SNIP_MAX_LEN:
                        break
                txt = sentence[2:]
                for key in keys:
                        txt = re.sub("(?i)(\W)(%s)(\W)" % key,
                                r"\1<b>\2</b>\3", txt)
                snip += txt + ".. "
        
        return title, snip

def render(msg):
        msg = cStringIO.StringIO(msg)
        query_msg = decode_netstring_fd(msg)
        score_msg = decode_netstring_fd(msg)
        rank_msg = decode_netstring_fd(msg)
        
        try:
                query = make_query.parse_qs(query_msg['query'])
                if 'offs' in query:
                        offs = int(query['offs'])
                else:
                        offs = 0
                query_q = query['q']
        except Exception, x:
                erlay.report("Invalid or missing query string: %s" %
                        query_msg['query'])
                traceback.print_exc()
                offs = 0
                query_q = ""

        layers = [None] * 10
        for layer_str in score_msg.itervalues():
                ainodex.deserialize_layer(layer_str, layers)

        results = [] 
        ranked = array.array("I", rank_msg['merged'])
        for dockey in ranked[offs * 2:offs * 2 + 20:2]:
                try:
                        title, snip = make_snippet(dockey, query_q, layers)
                        results.append((title, snip))
                except Exception, x:
                        erlay.report("Make snippet failed ("
                                "query:<%s> dockey:<%d>)" % (query_q, dockey))
                        traceback.print_exc()

        cues = cue_re.findall(query_q)
        keys = cue_re.split(query_q)
        if not cues:
                cues = keys

        return results_tmpl(int(rank_msg['num_hits']), query_q,\
                nice_re.sub("", " ".join(cues)),
                nice_re.sub("", " ".join(keys)), results, offs)
                


erlay.report("--- renderer starts ---")
ainodex.open()
t = time.time()
read_docs()
erlay.report("Doc index read in %dms" % ((time.time() - t) * 1000.0))

erlay.register_str(sys.argv[1], 'render')
erlay.serve_forever(globals()) 


