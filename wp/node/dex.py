
import os, sys, time, cStringIO
import ainodex
import make_query
import erlay

from netstring import *

LAYERS = map(int, "1 10 50 100 1000 5000 10000 20000 50000".split())
MIN_SCORE = 0.001

NAME = os.environ['NAME']
IBLOCK = int(os.environ['IBLOCK'])

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

        print "QUERY", query_msg

        t = time.time()
        hits_len, hits = ainodex.hits(map(int, query_msg['keys'].split()), 0)
        ret['num_hits'] = str(hits_len)
        erlay.report("Hits took %dms" %\
                        ((time.time() - t) * 1000.0))
        
        t = time.time()
        layers = [None] * 10
        for layer_str in layer_msg.itervalues():
                ainodex.deserialize_layer(layer_str, layers)
        erlay.report("Deser took %dms" %\
                        ((time.time() - t) * 1000.0))

        t = time.time()
        ret['ranked'] = ainodex.rank(hits, layers) 
        print "LL", len(ret['ranked'])
        erlay.report("Ranking <%s><%s> took %dms" % 
                (query_msg['keys'], query_msg['cues'], 
                        (time.time() - t) * 1000.0))

        return encode_netstring_fd(ret)



erlay.report("--- dex [%s/%d] starts ---" % (NAME, IBLOCK))

ainodex.open()
erlay.register_str(sys.argv[1], 'dex/iblock:%d' % IBLOCK)

erlay.serve_forever(globals()) 

