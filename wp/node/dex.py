
import os, sys, time, cStringIO, array, tempfile, re
import ainodex, ainopy
import make_query
import erlay

from netstring import *

site_re = re.compile("site(:|%3A)([a-z0-9\.]+)")

LAYERS = map(int, "1 10 50 100 1000 5000 10000 20000 50000".split())
MIN_SCORE = 0.001

NAME = os.environ['NAME']
IBLOCK = int(os.environ['IBLOCK'])

def read_sites():
	last_key = ainopy.did2key(ainopy.info()["Number of documents"] - 1)
	first_key = ainopy.did2key(0)
	sites = {}
	for l in file(NAME + ".sitehash"):
	        key, hash = map(int, l.split())
		key -= 1
		if key > last_key:
			break
		if key >= first_key:
			sites[key] = hash
	return sites

def filter_hits(hits_len, hits, prior_check = False, site_check = False, show_site = 0):
	if not seg_hashes:
		return hits_len, hits

	if prior_check:
		uniq = dict((seg_hashes[sid], sid)
			for sid in ainodex.hit_contents(hits)
				if ainopy.docinfo(ainopy.sid2did(sid))[1] == 1.0)
	else:
		uniq = dict((seg_hashes[sid], sid)
			for sid in ainodex.hit_contents(hits))
	
	if site_check:
		if show_site:
			for key, sid in uniq.items():
				h = sites[ainopy.did2key(ainopy.sid2did(sid))]
				if h != show_site:
					del uniq[key]
		else:
			u = {}
			for sid in uniq.itervalues():
				h = sites[ainopy.did2key(ainopy.sid2did(sid))]
				u[h] = sid
			uniq = u

	return len(uniq), ainodex.list_to_hits(uniq.values())

def get_normtable(msg):
        ret = {'iblock': str(IBLOCK),
               'normtable': ainodex.normtable()} 
        return encode_netstring_fd(ret)

def score(msg):
        ret = {}
        msg = decode_netstring_fd(cStringIO.StringIO(msg))
        #cueset_size, cues = ainodex.expand_cueset(
        #        map(int, msg['cues'].split()))

	cueset_size, cues = ainodex.hits(map(int, msg['cues'].split()), 0)
        cueset_size, cues = filter_hits(cueset_size, cues,\
		prior_check = True, site_check = True)
        ret['cueset_size'] = str(cueset_size)

        ok_layers = [i for i, maxf in enumerate(LAYERS)
                if min(maxf, cueset_size) / float(max(maxf, cueset_size)) 
                                > MIN_SCORE]

        if len(LAYERS) - 1 in ok_layers:
                ok_layers.append(len(LAYERS))
        print "OK", ok_layers, "CUES", cueset_size

        t = time.time()
        
        for i in ok_layers:
                layer = ainodex.new_layer(i, cues)
                ret[str(i)] = ainodex.serialize_layer(layer)
        
        erlay.report("Scoring <%s> took %dms" % 
                (msg['cues'], (time.time() - t) * 1000.0))

        return encode_netstring_fd(ret)

def rank(msg):
        t = time.time()
        ret = {}
        msg = cStringIO.StringIO(msg)
        query_msg = decode_netstring_fd(msg)
        layer_msg = decode_netstring_fd(msg)
        erlay.report("Rank init took %dms" %\
                        ((time.time() - t) * 1000.0))

        print >> sys.stderr, "QUERY", query_msg

	if query_msg['mods'] and query_msg['mods'].startswith("site:"):
		ok_site = hash(query_msg['mods'][5:])
		print >> sys.stderr, "SHOW SITE", query_msg['mods'], ok_site
	else:
		ok_site = 0

        t = time.time()
	hits_len, hits = ainodex.hits(map(int, query_msg['keys'].split()), 0)
        ret['num_hits'] = str(hits_len)
        hits_len, hits = filter_hits(hits_len, hits,\
		site_check = True, prior_check=True, show_site=ok_site)
        erlay.report("Hits took %dms" %\
                        ((time.time() - t) * 1000.0))
       
       	print "HITS_LEN", hits_len
        t = time.time()
        layers = [None] * 10
        for layer_str in layer_msg.itervalues():
                ainodex.deserialize_layer(layer_str, layers)
        erlay.report("Deser took %dms" %\
                        ((time.time() - t) * 1000.0))

	#kkeys = map(lambda x: ainopy.did2key(ainopy.sid2doc(x)[0]), ainodex.hit_contents(hits))


        t = time.time()
        ret['ranked'] = ainodex.rank(hits, layers) 
	print >> sys.stderr, "RANKED", array.array("I", ret["ranked"])[:20:2]
	#for key in array.array("I", ret["ranked"])[:20:2]:
	#if key not in okkeys:
	#	print >> sys.stderr, "NOT IN OK", key

        print "LL", len(ret['ranked'])
        erlay.report("Ranking <%s><%s> took %dms" % 
                (query_msg['keys'], query_msg['cues'], 
                        (time.time() - t) * 1000.0))

        return encode_netstring_fd(ret)



erlay.report("--- dex [%s/%d] starts ---" % (NAME, IBLOCK))

ainodex.open()
ainopy.open()

try:
	seg_hashes = map(int, file("%s.%d.seghash" % (NAME, IBLOCK)).readlines())
	erlay.report("Segment hashes read. Duplicate checking enabled.")
except:
	erlay.report("Could not open segment hashes. Duplicate checking disabled.")
	seg_hashes = None

try:
	sites = read_sites()
	erlay.report("Site hashes read. Site checking enabled.")
except:
	erlay.report("Could not open site hashes. Site checking disabled.")
	sites = None

if "TESTDEX" in os.environ:
	globals()[sys.argv[1]](file(sys.argv[2]).read())
else:
	erlay.register_str(sys.argv[1], 'dex/iblock:%d' % IBLOCK)
	erlay.serve_forever(globals()) 

