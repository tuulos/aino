import sys, re, os, zlib

duplo = {}
ANPPAFIX = True
key = 1
make_db = False

head_re = re.compile("(.*?)\n---\n", re.S)
mime_re = re.compile("MIME text/html.*")
url_re = re.compile("URL (.*)")
id_re = re.compile("ID (.*)")

if int(os.environ['MAKEDB']):
	docdump = file("%s.docs" % os.environ['NAME'], "w")
	docdump_idx = file("%s.docs.index" % os.environ['NAME'], "w")
	make_db = True

maxdoc = int(sys.argv[1])
while key < maxdoc:
        ll = sys.stdin.readline()
	if not ll:
		sys.exit(0)
        while not ll.strip():
                ll = sys.stdin.readline()
		if not ll:
			sys.exit(0)
	ll = int(ll)
        if ANPPAFIX:
                doc = sys.stdin.read(ll)[:-1]
        else:
                doc = sys.stdin.read(ll)
       	
	head = head_re.search(doc).group(1)
	id = id_re.search(head).group(1)
	url = url_re.search(head).group(1)
	
	if ll > 1024**2:
		print >> sys.stderr, "Skipping doc <%s> <%s>: %fMB" % (id, url, ll / 1024**2)
		continue
       
	if not mime_re.search(head):
		print >> sys.stderr, "Skipping a non-html doc <%s> <%s>" % (id, url)
		continue
	
	out = "ID %s\n----\n%s" % (key, doc)
	print "%d\n%s\n" % (len(out), out)
	key += 1

	#XXX: use separate make_db.py instead!
	#if make_db:
	#	dd = "%s\000%s" % (url, doc)
	#	dd = zlib.compress(dd, 3)
	#	print >> docdump_idx, "%d %d %d" % (key, docdump.tell(), len(dd))
	#	docdump.write(dd)

print >> sys.stderr, "suckdoc done"
os.system("pkill -KILL -f docstore2docstream") 
