#/*
# *   ainopy/ainopeek.py
# *   
# *   Copyright (C) 2006-2008 Ville H. Tuulos
# *
# *   This program is free software; you can redistribute it and/or modify
# *   it under the terms of the GNU General Public License as published by
# *   the Free Software Foundation; either version 2 of the License, or
# *   (at your option) any later version.
# *
# *   This program is distributed in the hope that it will be useful,
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# *   GNU General Public License for more details.
# *
# *   You should have received a copy of the GNU General Public License
# *   along with this program; if not, write to the Free Software
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# *
# */

from os import environ as env
import readline, sys, cmd
import ainopy

class AinoCmd(cmd.Cmd):
        
        def default(self, line):
                l = line.split()
                try:
                        if l[0] == "open":
                                print "Index already open"
                                return
                        print apply(getattr(ainopy, l[0]), map(int, (l[1:])))
                except AttributeError:
                        print "No such command: %s" % l[0]
                except Exception, x:
                        print x

        def help_source(self):
                print "Batch-processes commands from the given file"
                 
        def do_source(self, args):
                try:
                        f = file(args)
                except:
                        self.help_source()
                        return

                for l in f:
                        self.onecmd(l)

        def do_help(self, args):
                if not args:
                        print\
"""
Available commands:
open, inva, xids, fw, pos, sid2doc, did2doc, did2key, info, stats, quit

Type help [command] to get more information
"""
                        return
                try:
                        if args == "quit":
                                self.help_quit()
                                return
                        elif args == "source":
                                self.help_source()
                                return
                        else:
                                print getattr(ainopy, args).__doc__
                except AttributeError:
                        print "No such command: %s" % args
                        self.do_help(None)

        def help_quit(self):
                print "Quits"

        def do_quit(self, args):
                sys.exit(0)

                        
if 'NAME' not in env or 'IBLOCK' not in env:
        print "Please set NAME and IBLOCK"
        sys.exit(1)

ainopy.open()

prompt = AinoCmd("SpotSim!")
prompt.prompt = "aino> "
if len(sys.argv) > 1:
        prompt.onecmd("source %s" % sys.argv[1])

prompt.cmdloop()


