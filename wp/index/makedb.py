import re, sys, cStringIO, os, bsddb, zlib
#from mwlib import dummydb, parser, expander, uparser

NOF_DOCS = int(os.environ['NOF_DOCS'])

write_db = False
if 'WRITE_DB' in os.environ:
#	db = bsddb.hashopen("wiki.db", "n")
	write_db = True

docdump = file("%s.docs" % os.environ['NAME'], "w")
docdump_idx = file("%s.docs.index" % os.environ['NAME'], "w")

title_re = re.compile('<title>(.*?)</title')
txt_re = re.compile('<text xml:space="preserve">(.*?)</text>', re.M | re.S)
key = 0
titles = []

curly_re1 = re.compile(r"\{\{.*?\}\}", re.S)
curly_re2 = re.compile(r"\{.*?\}", re.S)

bracket_re1 = re.compile(r"\[\[([\?:\w\s\-'\(\)]+)]\]", re.S)
bracket_re2 = re.compile(r"\|([\?:\w\s\-'\(\)]+)\]\]", re.S)
bracket_re3 = re.compile(r"\[\[[\?:\w\s\-'\(\)]+\|", re.S)
image_re = re.compile(r"\[\[[Ii]mage:.+?\| ", re.S)

def process_page(p):
        global key
        txt = txt_re.search(p)
        title = title_re.search(p).group(1)
	if ":" in title:
		#print >> sys.stderr, "Skipped", title
		return
        if txt:
                unic = txt.group(1).decode("utf-8")
		if "#REDIRECT" in unic or "#redirect" in unic:
			return

                key += 1 
		if not key % 1000:
			print >> sys.stderr, key

		#doc = uparser.parseString(raw = unic, title = "test").asText()
		doc = unic.encode("iso-8859-1", "replace")

		#doc = unic.encode("iso-8859-1", "replace")
		#doc = re.sub(bracket_re1, r" \1 ", doc)
		#doc = re.sub(bracket_re2, r"| \1 ", doc)
		#doc = re.sub(bracket_re3, r" ", doc)
		#doc = re.sub(image_re, r" ", doc)
		#doc = re.sub(curly_re1, r"", doc)
		#doc = re.sub(curly_re2, r"", doc)
		#doc = re.sub("\&lt\;.*?\&gt\;", r"", doc)
		#doc = re.sub(r"&.*?;", r"", doc)
		#doc = re.sub(r"http://.*? ", r"", doc)
		#doc = re.sub(r"[\]\[]", r"", doc)
		#doc = re.sub(r"===(.*?)===", r"\1.", doc)
		#doc = re.sub(r"\?+", r"?", doc)
		#doc = re.sub(r"\s+", r" ", doc)

		if write_db:
			dd = "%s\000%s" % (title.replace("\000", " "), doc)
			dd = zlib.compress(dd, 3)
			print >> docdump_idx, "%d %d %d" % (key, docdump.tell(), len(dd))
			docdump.write(dd)
                
		out = "ID %d\n URL foo\n---\n%s" % (key, doc)
                print "%d\n%s\n" % (len(out), out)
		 
                if key == NOF_DOCS:
                        sys.exit(0)

page = None

for line in sys.stdin:
        if not page and line.lstrip().startswith("<page>"):
                page = cStringIO.StringIO()

        elif page and line.lstrip().startswith("</page>"):
                process_page(page.getvalue())
                page = None
        elif page:
                page.write(line)

