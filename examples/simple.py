import sys, array
import ainodex

ainodex.open()

if len(sys.argv) < 2:
	print "Usage: simple [key] [cue]"
	sys.exit(1)

keys = ainodex.token2ixeme(sys.argv[1])
cues = ainodex.token2ixeme(sys.argv[2])
print "KEYS", keys, "CUES", cues

hits_len, hitset = ainodex.hits([keys], 0)
cues_len, cueset = ainodex.hits([cues], 0)

print "%s occurs in %d segments" % (sys.argv[1], hits_len)
print "%s occurs in %d segments" % (sys.argv[2], cues_len)

# Word frequencies
normtable = ainodex.normtable_to_judy(ainodex.normtable())

# Compute how many times tokens co-occur with the cueset
layers = [ainodex.new_layer(i, cueset) for i in range(10)]
for layer in layers:
	# Compute token scores
	ainodex.normalize_layer(layer, normtable, cues_len)

ranked = ainodex.rank(hitset, layers)
doc_keys = array.array("I", ranked)[:20:2]
doc_scores = array.array("f", ranked)[1:20:2]

print zip(doc_keys, doc_scores)
