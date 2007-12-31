
import sys, time, cStringIO
import ainodex
import make_query
import erlay

from netstring import *

normtables = {}
normtable = None

MIN_SCORE = 0.001

def add_normtable(msg):
        global normtable
        msg = cStringIO.StringIO(msg)
        while True:
                try:
                        iblock_normtable = decode_netstring_fd(msg)
                except EOFError:
                        break
                normtables[iblock_normtable['iblock']] =\
                        iblock_normtable['normtable']

        normtable = ainodex.normtable_to_judy(
                "".join(normtables.itervalues()))

        erlay.report("Got normtables from iblocks <%s>" %
                " ".join(normtables.keys()))
        return "ok"


def parse_query(msg):
        query = None
        try:
                query = make_query.parse_qs(msg)
                keys, cues = make_query.make_query(query['q'])
                keys = " ".join(map(str, keys))
                cues = " ".join(map(str, cues))
        except Exception, x:
                erlay.report("Query parsing failed: %s" % x)
                if query:
                        erlay.report("Failed query: %s" % query)
                keys = []
                cues = []
                query = ""

        erlay.report("Query string: <%s> Keys: <%s> Cues: <%s>" %\
                (query, keys, cues))
        
        return encode_netstring_fd({'keys': keys, 'cues': cues, 
                        'query': msg})


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
        return encode_netstring_fd({'merged': top, 'num_hits': str(num_hits)})


erlay.report("--- merger starts ---")

ainodex.open()
erlay.register_str(sys.argv[1], 'dex/merger')

erlay.serve_forever(globals()) 


