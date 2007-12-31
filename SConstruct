
opts = Options('configure.py')
#opts.Add(BoolOption('CROSSPLATFORM',\
#        'Enable binary compatibility of the indices across x86_32 and x86_64.',
#        0))
#opts.Add(BoolOption('SECTION_OFFS64',\
#        'Enable index sections larger than 4GB.',
#        0))

opts.Add(BoolOption('DEBUG',\
        'Enable some (costly) asserts for debugging',
        "no"))

#opts.Add(BoolOption('DOCBDB',\
#        'Enable BerkeleyDB-based DOCDB',
#        "yes"))

opts.Add(BoolOption('AINOPY',\
       'Compile Python-interface',
        "yes"))

#opts.Add(PathOption('SKIN',\
#        'Path to result page html skin file.',
#        'cgi/simple.skin'))
#opts.Add(BoolOption('LANG_SPECIFIER',\
#        'Enable lang: specifier in queries. Requires language detection.',
#        "yes"))

path = ['/usr/bin', '/bin']
g_env = Environment(options = opts,
                    CPPPATH = ['.','#/lib'],
                    PATH = path, 
                    CCFLAGS = ['-fPIC', '-Wall', '-O3', '-g'], 
                    LIBPATH = ['#/lib','.', '#/index'],
                    CPPDEFINES = {'CROSSPLATFORM' : '${CROSSPLATFORM}',
                                  'DEBUG': '${DEBUG}'})
        
Help(opts.GenerateHelpText(g_env))

modules = ['lib/SConstruct',
           'preproc/SConstruct',
           'index/SConstruct',
           'lang/SConstruct',
           'circus/SConstruct']

conf = Configure(g_env)

if not conf.CheckHeader('Judy.h', language='C'):
	print 'Did not find Judy.h, exiting!'
	Exit(1)

if not conf.CheckLib('Judy', autoadd=0):
	print 'Did not find libJudy.a or Judy.lib, exiting!'
	Exit(1)

if not conf.CheckHeader('zlib.h', language='C'):
	print 'Did not find zlib.h, exiting!'
	Exit(1)

if not conf.CheckLib('z', autoadd=0):
	print 'Did not find libz.a or z.lib, exiting!'
	Exit(1)

if not conf.CheckLib('readline', autoadd=0):
	print 'Did not find libreadline.a, exiting!'
	Exit(1)

#if g_env['DOCBDB']:
#        if not conf.CheckLib('db', autoadd=0):
#                print "DOCBDB enabled but libdb.a not found"
#                Exit(1)

if g_env['AINOPY']:
        modules.append("ainopy/SConstruct")
        if not conf.CheckHeader('python2.4/Python.h', language='C'):
	        print "AINOPY enabled but python2.4/Python.h not found"
        	Exit(1)
                
                
g_env = conf.Finish()
Export('g_env')
SConscript(modules)
