#!/bin/python
import sys, re
import ainodex

# keep these in sync with ixemes.h
XID_QOP_SEQ = 200000006
XID_QOP_PHA = 200000007


# this function stolen from urllib.py
def unquote(s):
        """unquote('abc%20def') -> 'abc def'."""
        mychr = chr
        myatoi = int
        list = s.split('%')
        res = [list[0]]
        myappend = res.append
        del list[0]
        for item in list:
                if item[1:2]:
                        try:
                                myappend(mychr(myatoi(item[:2], 16))
                                + item[2:])
                        except ValueError:
                                myappend('%' + item)
                else:
                        myappend('%' + item)
        return "".join(res)


# this function stolen from cgi.py
def parse_qs(qs, strict_parsing = True, keep_blank_values = False):
        pairs = [s2 for s1 in qs.split('&') for s2 in s1.split(';')]
        r = []
        for name_value in pairs:
                if not name_value and not strict_parsing:
                        continue
                nv = name_value.split('=', 1)
                if len(nv) != 2:
                        if strict_parsing:
                                raise ValueError, "bad query field: %r" %\
                                        (name_value,)
                        # Handle case of a control-name with no equal sign
                        if keep_blank_values:
                                nv.append('')
                        else:
                                continue
        
                if len(nv[1]) or keep_blank_values:
                        name = unquote(nv[0].replace('+', ' '))
                        value = unquote(nv[1].replace('+', ' '))
                        r.append((name, value))
        return dict(r)


def find_id(token):
        xids = [ainodex.token2ixeme(w) for w in re.split('(?u)\W+', token) if w]
        return filter(lambda x: x, xids)

def add_cue(token):
        global cues
        cues += find_id(token)
        return True

def add_to_phrase(token):
        global active_phrase
        
        if not active_phrase:
                active_phrase.append(XID_QOP_SEQ)
        
        if token == '"':
                active_phrase.append(XID_QOP_PHA)
                #phrases.append(tuple(active_phrase))
                #active_phrase = []
                return True
        else:
                i = token.find('"')
                if i == -1:
                        active_phrase += find_id(token)
                        return False
                else:
                        active_phrase += find_id(token[:i])
                        active_phrase.append(XID_QOP_PHA)
                        #phrases.append(tuple(active_phrase))
                        #active_phrase = []
                        return True


def add_keyword(token):
        global keys
        keys += find_id(token)
        return False

def add_modifier(token):
        modifiers.append(token)
	return True

def flatten(lst):
        r = []
        for x in lst:
                if type(x) == tuple:
                        r += x
                else:
                        r.append(x)
        return r

def make_query(q):
        global keys, cues, phrases, modifiers, active_phrase

        keys = []
        cues = []
        #phrases = []
        modifiers = []
        active_phrase = []
        phrase_target = None

        mode = add_keyword
        for token in q.split():
                if mode == add_keyword:
                        if token.startswith('/"'):
                                mode = add_to_phrase
                                token = token[2:]
                                phrase_target = cues
                        if token[0] == '/':
                                mode = add_cue
                                token = token[1:]
                        elif token[0] == '"':
                                mode = add_to_phrase
                                token = token[1:]
                                phrase_target = keys
                        elif token.find(':') != -1:
                                mode = add_modifier
                if mode(token):
                        mode = add_keyword
                        if phrase_target != None:
                                phrase_target.append(tuple(active_phrase))
                                phrase_target = None
                                active_phrase = []

        if phrase_target != None:
                active_phrase.append(XID_QOP_PHA)
                phrase_target.append(tuple(active_phrase))

        # canonize query order
        #keys += modifiers
        keys.sort(reverse = True)
        cues.sort(reverse = True)
       
        if not cues:
                cues = keys
        #        for phrase in phrases:
        #                cues += phrase[1:-1]

        return flatten(keys), flatten(cues), flatten(modifiers)
        

