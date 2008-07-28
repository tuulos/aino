import ainopy, ainodex

ainodex.open()
ainopy.open()
rev_ixi = None

def _to_xid(t):
        if type(t) == str:
                xid = ainodex.token2ixeme(t)
        elif type(t) == int:
                xid = t
        else:
                raise ValueError("Argument not a string or integer")
        if not xid:
                raise KeyError("No such ixeme")
        return xid

def inverted_list(t):
        xid = _to_xid(t)
        prev_did = -1
        res = []
        for sid in ainopy.inva(xid):
                did = ainopy.sid2did(sid)
                if did != prev_did:
                        res.append(did)
                        prev_did = did
        return res

def bag(did):
        xids = []
        for sid in ainopy.did2doc(did):
                for layer in range(10):
                        xids += ainopy.fw(layer, sid)
        return xids

def id2token(id):
        global rev_ixi
        if not rev_ixi:
                rev_ixi = ainodex.ixicon(0)
        return ainodex.ixicon_entry(rev_ixi, id)

def frequency(t):
        return ainopy.freq(_to_xid(t))

