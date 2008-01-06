
import re, time, os, sys, cStringIO, traceback
import array, zlib, md5, string, operator
import ainodex, erlay, make_query
from web import template, net

from netstring import *

SNIP_MAX_LEN = 500

site_re = re.compile("site(:|%3A)([a-z0-9\.]+)")
cue_re = re.compile("/\w+|/\".+?\"", re.U)
nice_re = re.compile("[^\w\" ]", re.U)

cand_re = re.compile("[\.\*\n].+?\.", re.U)
results_tmpl = template.Template(file("results.tmpl").read(), filter = net.websafe)
template.Template.globals['range'] = range
template.Template.globals['max'] = max
template.Template.globals['min'] = min
template.Template.globals['len'] = len
template.Template.globals['cut'] = lambda x: x[:100]

class InvalidHit:
	pass

def url2site(url):
	site, path = (url[7:] + '/').split("/", 1)
	return ".".join(site.split(".")[-2:])

def read_docs():
        global doc_db, doc_idx
        doc_db = file("%s.docs" % os.environ['NAME'])
	doc_idx = {}
	for l in file("%s.docs.index" % os.environ['NAME']):
		key, val = l.split(" ", 1)
        	doc_idx[int(key)] = val

def make_snippet(dockey, query, layers, seen_sites, seen_md5s):
	#erlay.report("QEURYE (%s) <%s> <%s>" % (type(query), query, dockey))
        offs, size = map(int, doc_idx[dockey].split())
        doc_db.seek(offs)
        url, title, doc = zlib.decompress(doc_db.read(size)).split('\0')
	title = title.decode("iso-8859-1")
	doc = doc.decode("iso-8859-1")
	site = url2site(url)
	
	if site in seen_sites:
		raise InvalidHit
	seen_sites[site] = True
	
	dig = md5.md5(doc.encode("iso-8859-1", "ignore")).digest()
	if dig in seen_md5s:
		raise InvalidHit
	seen_md5s[dig] = True

	title = title.strip()
	if len(title) > 50 or len(title) == 0:
		title = title[:50] + "..."
	keys = filter(lambda x: x, [re.sub("(?u)\W", "", key) for key in query.split()])
	erlay.report("KEYS %s" % keys)

        unic = re.sub("\\.*?\]", "", doc)
        unic = re.sub("[\[\]\|]", "", unic)
        candidates = re.findall(cand_re, unic)
        scores = []
        
        for sentence in candidates:
                txt = re.sub("(?u)\W", " ", sentence)
		txt = txt.encode("iso-8859-1")
                xidbag = filter(lambda x: x,
                        map(ainodex.token2ixeme, txt.split()))
                s = sum(1.0 for key in keys 
			if re.search("(?ui)\W%s\W" % key, txt))
                s += sum(ainodex.score_list(xidbag, layer)
                        for layer in layers if layer)
                scores.append((s, sentence))
        scores.sort(reverse = True)
        
        snip = ""
        for score, sentence in scores:
                if len(snip) + len(sentence) > SNIP_MAX_LEN:
                        break
                txt = sentence[1:].strip()
		for key in keys:
                        txt = re.sub("(?iu)(\W)(%s)(\W)" % key,
                                r"\1<b>\2</b>\3", txt)
                snip += txt + ".. "
       
	if len(snip) > SNIP_MAX_LEN:
		snip = snip[:SNIP_MAX_LEN] + "..."

        return title, url, snip

def show_best_ix(layers):
	top = ainodex.top_ixemes(layers, 100)
	snip = "<br>".join(map(lambda x: "%s %2.3f" %
		(ainodex.ixicon_entry(rixicon, x[0]), x[1]), top))
	return "Top-100 tokens", "#", "#", snip

def render(msg):
        msg = cStringIO.StringIO(msg)
        query_msg = decode_netstring_fd(msg)
        score_msg = decode_netstring_fd(msg)
        rank_msg = decode_netstring_fd(msg)
		
	site_check = True
	show_score = False
        
        try:
                query = make_query.parse_qs(query_msg['query'])
                if 'offs' in query:
                        offs = int(query['offs'])
                else:
                        offs = 0
                query_q = query['q']
		
		if query_msg['mods']:
			if query_msg['mods'].startswith("site"):
				site_check = False
			elif query_msg['mods'].startswith("score"):
				show_score = True

        except Exception, x:
                erlay.report("Invalid or missing query string: %s" %
                        query_msg['query'])
                traceback.print_exc()
                offs = 0
                query_q = ""

        layers = [None] * 10
        for layer_str in score_msg.itervalues():
                ainodex.deserialize_layer(layer_str, layers)

	ranked = array.array("I", rank_msg['merged'])[::2]
	results = [] 
	if show_score:
       		results.append(show_best_ix(layers))
		hits = len(ranked) + 1
		site_check = True
	else:
		hits = 0
		valid_hits = 0
		seen_sites = {}
		seen_md5s = {}

	while hits < len(ranked):
		dockey = ranked[hits]
		hits += 1

                try:
			# XXX: dockey + 1 is a quick fix for aino.fi!
                        title, url, snip =\
				make_snippet(dockey + 1, query_q, layers,\
					seen_sites, seen_md5s)
			if not site_check:
				seen_sites = {}

                        if valid_hits >= offs:
				site = url2site(url)
				results.append(tuple(map(lambda x:
					x.encode("iso-8859-1"), 
						(title, url, site, snip))))
			valid_hits += 1
			if len(results) == 10:
				break
		except InvalidHit:
			continue
                except Exception, x:
                        erlay.report("Make snippet failed ("
                                "query:<%s> dockey:<%d>)" % (query_q, dockey))
                        traceback.print_exc()

        cues = cue_re.findall(query_q)
	keys = map(lambda x: nice_re.sub("", x).strip(),\
		cue_re.split(site_re.sub("", query_q)))

	#erlay.report("CUES <%s>" % cues)
	#erlay.report("KEYS <%s>" % keys)

	if site_check:
		base_q = " ".join(keys) + " ".join(cues)
	else:
		base_q = ""
	
	#erlay.report("BASE <%s>" % base_q)
	if not cues:
                cues = keys

        return results_tmpl(int(rank_msg['num_hits']), query_q, base_q,\
                nice_re.sub("", " ".join(cues)),
                nice_re.sub("", " ".join(keys)), results, offs)


erlay.report("--- renderer starts ---")
ainodex.open()
rixicon = ainodex.ixicon(0)

t = time.time()
read_docs()
erlay.report("Doc index read in %dms" % ((time.time() - t) * 1000.0))

if "TESTDEX" in os.environ:
        globals()[sys.argv[1]](file(sys.argv[2]).read())
else:
	erlay.register_str(sys.argv[1], 'render')
	erlay.serve_forever(globals()) 


