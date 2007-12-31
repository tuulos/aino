
ERLAY=/home/tuulos/src/erlay/

erl -sname erlay +K true -smp on -pa $ERLAY/ebin -pa src -boot $ERLAY/erlay -erlay scgi_port 2222 -erlay erlay_port 3333
