
import sys, time, cStringIO, array, os
import ainodex
import make_query
import erlay

from netstring import *

normtables = {}
normtable = None
last_normtable_update = 0
normtables_need_update = False

MIN_SCORE = 0.001
NORMTABLE_UPDATE = 30.0

def update_normtable():
        global normtable, last_normtable_update, normtables_need_update
	normtables_need_update = False
	erlay.report("Updating normtable")
        normtable = ainodex.normtable_to_judy(
			"".join(normtables.itervalues()))
	last_normtable_update = time.time()

def add_normtable(msg):
        global normtables_need_update
	c = time.time()
        msg = cStringIO.StringIO(msg)
        while True:
                try:
                        iblock_normtable = decode_netstring_fd(msg)
                except EOFError:
                        break
                normtables[iblock_normtable['iblock']] =\
                        iblock_normtable['normtable']
	
	erlay.report("This took %dms" % ((time.time() - c) * 1000.0))
	normtables_need_update = True

	if time.time() - last_normtable_update > NORMTABLE_UPDATE:
		update_normtable()

        erlay.report("Got normtables from iblocks <%s>" %
                " ".join(normtables.keys()))
        return "ok"


def parse_query(msg):
	if normtables_need_update and\
		time.time() - last_normtable_update > NORMTABLE_UPDATE:
		update_normtable()

        query = None
        try:
                query = make_query.parse_qs(msg)
                keys, cues, mods = make_query.make_query(query['q'])
                keys = " ".join(map(str, keys))
                cues = " ".join(map(str, cues))
        	mods = " ".join(mods)
	except Exception, x:
                erlay.report("Query parsing failed: %s" % x)
                if query:
                        erlay.report("Failed query: %s" % query)
                keys = ""
                cues = ""
		mods = ""
                query = ""

        erlay.report("Query string: <%s> Keys: <%s> Cues: <%s>" %\
                (query, keys, cues))
        
        return encode_netstring_fd({'keys': keys, 'cues': cues, 
			'mods': mods, 'query': msg})


def merge_scores(msg):
        msg = cStringIO.StringIO(msg)

        layers = [None] * 10
        cueset_size = 0

        while True:
                try:
                        iblock_layers = decode_netstring_fd(msg)
                except EOFError:
                        break

                cueset_size += int(iblock_layers['cueset_size'])
                del iblock_layers['cueset_size']
		
                for layer_data in iblock_layers.itervalues():
                        offs, layer_id, layer =\
                                ainodex.deserialize_layer(
                                        layer_data, layers)

	#XXX: Since ixemes are allocated on different layers on each layer,
	# we must make sure that the ixeme counts match on every layer. This
	# could be easily avoided if ixemes were on the same layers on all
	# iblocks.  This should be easy to fix.
	t = time.time()
	ainodex.sync_layers(layers)
	erlay.report("Syncing layers took %dms" %\
	                   ((time.time() - t) * 1000.0))

        print "CUE", type(cueset_size), cueset_size
        for layer in layers:
                if layer:
                        ainodex.normalize_layer(layer, normtable, cueset_size)

        layers = [(str(i), ainodex.serialize_layer(layer))
                        for i, layer in enumerate(layers) if layer]

        return encode_netstring_fd(dict(layers))


def merge_ranked(msg):
        msg = cStringIO.StringIO(msg)
        r = cStringIO.StringIO()
        num_hits = 0
        while True:
                try:
                        iblock_ranked = decode_netstring_fd(msg)
                        r.write(iblock_ranked['ranked'])
                        num_hits += int(iblock_ranked['num_hits'])
                except EOFError:
                        break
        top = ainodex.merge_ranked(r.getvalue())
	erlay.report("Top keys: %s" % array.array("I", top)[:20:2])

        return encode_netstring_fd({'merged': top, 'num_hits': str(num_hits)})


erlay.report("--- merger starts ---")

ainodex.open()

if "TESTDEX" in os.environ:
        globals()[sys.argv[1]](file(sys.argv[2]).read())
else:
	erlay.register_str(sys.argv[1], 'dex/merger')
	erlay.serve_forever(globals()) 



