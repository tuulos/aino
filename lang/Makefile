
CC     = gcc
CPP    = cpp
SBALL  = q/stem.o q/utilities.o q/api.o
CFLAGS = -g -O3 -L ../lib -I ../lib

STEMS  = $(subst stems/,, $(wildcard stems/*.sbl))
LANGS  = $(patsubst %.sbl,%, $(STEMS))

all: langs testers lang_detect lemmatizer

clean:
		rm -Rf X q/*.o p/*.o q/LANG_* lang_conf.cfg\
		snowball_lemmatize.c

snowball:	
		mkdir X; p/make &&\
		cd q &&\
		$(CC) $(CFLAGS) -c *.c &&\
		cd ..

%.sbl.o:		
		@echo "Lang $*"
		X/snowball stems/$*.sbl -ep $*_ -o q/$*

%.test.o:
		$(CC) $(CFLAGS) -I q -D LANG=$* stem_test.c\
		q/$*.c $(SBALL) -o test_$*

testers:	langs  $(patsubst %.sbl,%.test.o, $(STEMS))

langs:		snowball $(patsubst %.sbl,%.sbl.o, $(STEMS))

lang_conf.cfg:	
		../utils/jacktherepeater.py lang_conf.ctmpl $(LANGS) |\
		$(CPP) -I ../lib > lang_conf.cfg
		
lang_detect:    lang_conf.cfg
		$(CC) $(CFLAGS) -I . -L . -I ../plister/ -I ../lib -L ../lib lang_detect.c ../plister/plister_lib.c -lfinnu -ltextcat -lJudy -o lang_detect

snowball_lemmatize.c:
		../jacktherepeater.py snowball_lemmatize-tmpl.c $(LANGS) > snowball_lemmatize.c
		
lemmatizer:	langs snowball_lemmatize.c
		$(CC) $(CFLAGS) -I q -I ../plister lemmatizer.c snowball_lemmatize.c ../plister/stats_lib.c q/LANG*.c q/stem.o q/utilities.o q/api.o -lfinnu -lJudy -o lemmatizer

