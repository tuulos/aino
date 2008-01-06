import os, zlib, re, sys

head_re = re.compile("(.*?)\n---\n(.*)", re.S)
title_re = re.compile("TITLE (.*)")
id_re = re.compile("ID (.*)")
url_re = re.compile("URL (.*)")

docdump = file("%s-ok.docs" % os.environ['NAME'], "w")
docdump_idx = file("%s-ok.docs.index" % os.environ['NAME'], "w")


while True:
        ll = sys.stdin.readline()
	if not ll:
		sys.exit(0)
        while not ll.strip():
                ll = sys.stdin.readline()
		if not ll:
			sys.exit(0)
        doc = sys.stdin.read(int(ll))
	
	m = head_re.search(doc)
	head = m.group(1)
	body = m.group(2)
	id = id_re.search(head).group(1)
	url = url_re.search(head).group(1)
	m = title_re.search(head)
	if m:
		title = m.group(1)
	else:
		title = ""

	dd = "%s\000%s\000%s" % tuple(map(lambda x: x.replace("\000", ""), (url, title, body)))
	dd = zlib.compress(dd, 3)
	print >> docdump_idx, "%s %d %d" % (id, docdump.tell(), len(dd))
	docdump.write(dd)

	print "%d\n%s\n" % (len(doc), doc)
	

