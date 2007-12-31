
import cStringIO

MAX_LEN_STRING = 10
MAX_PACKET_LEN = 100 * 1024 * 1024

def _read_string(msg, i):
        j = msg.index(" ", i)
        len = int(msg[i: j])
        j += 1
        return (j + len + 1, msg[j: j + len])


def encode_netstring_str(d):
        msg = cStringIO.StringIO()
        for k, v in d.iteritems():
                msg.write("%d %s %d %s\n" %\
                        (len(k), k, len(v), v))
        return msg.getvalue()


def encode_netstring_fd(d):
        s = encode_netstring_str(d)
        return "%d\n%s" % (len(s), s)


def decode_netstring_str(msg):
        i = 0
        d = {}
        while i < len(msg):
                i, key = _read_string(msg, i)
                i, val = _read_string(msg, i)
                d[key] = val
        return d


def decode_netstring_fd(fd):
        i = 0
        lenstr = ""
        while 1:
                c = fd.read(1)
                if not c:
                        raise EOFError()
                elif c.isspace():
                        break
                lenstr += c
                i += 1
                if i > MAX_LEN_STRING:
                        raise "Length string too long"
       
        if not lenstr:
                raise EOFError()
       
        llen = int(lenstr)
        if llen > MAX_PACKET_LEN:
                raise "Will not receive %d bytes" % llen
        
        return decode_netstring_str(fd.read(llen))
