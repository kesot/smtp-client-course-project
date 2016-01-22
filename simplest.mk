all: test_client report.pdf
src/client-fsm.c: src/client.def
	cd src && autogen client.def
include/client-fsm.h: src/client.def
	cd src && autogen client.def
	mv -f src/client-fsm.h include
dot/%_def.dot: src/%.def
	utils/fsm2dot $< > $@
tex/include/%_dot.tex: dot/%.dot
	dot2tex -ftikz --autosize --crop  $< > $@
tex/include/%_dot.pdf: tex/include/%_dot.tex
	pdflatex -interaction=nonstopmode -output-directory tex/include $<
tex/include/%_re.tex: include/regexp.h
	utils/re2tex include/regexp.h tex/include
bin/%.o: src/%.c  
	gcc -c -Iinclude -Wall -o $@ $<
test_client: src/client-fsm.o src/key-listener.o src/log.o src/maildir.o src/main.o src/opts.o src/protocol.o src/regexp.o src/utils.o
	gcc -o $@ -lopts $^
report.pdf: tex/header.tex tex/report.tex doxygen tex/include/client_def_dot.pdf tex/include/Makefile_1_dot.pdf tex/include/re_cmd_quit_re.tex tex/include/re_cmd_user_re.tex tex/include/cflow01_dot.pdf tex/include/cflow02_dot.pdf 
	cd tex && pdflatex -interaction=nonstopmode report.tex && pdflatex -interaction=nonstopmode report.tex && cp report.pdf ..
simplest.mk: Makefile
	sed 's/$//'  Makefile | utils/makesimple  > simplest.mk
dot/cflow01.dot: src/client-fsm.c src/key-listener.c src/log.c src/maildir.c src/main.c src/protocol.c src/regexp.c src/utils.c
	cflow --level "0= " $^ | grep -v -f cflow.ignore | utils/cflow2dot > $@
dot/cflow02.dot: src/protocol.c src/regexp.c src/client-fsm.c
	cflow --level "0= " $^ | grep -v -f cflow.ignore | utils/cflow2dot > $@
dot/Makefile_1.dot: utils/makefile2dot simplest.mk
	utils/makefile2dot test_client < simplest.mk > dot/Makefile_1.dot
.PHONY: tests
tests: test_units test_memory test_style test_system
.PHONY: test_units
test_units:
	echo Сделайте сами на основе check.sourceforge.net
.PHONY: test_style
test_style:
	echo Сделайте сами на основе astyle.sourceforge.net или checkpatch.pl
	echo Для инженеров -- не обязательно
.PHONY: test_memory
test_memory: test_client
	echo Сделайте сами на основе valgrind
.PHONY: test_system
test_system: test_client
	echo Сделайте сами на основе тестирования через netcat и/или скриптов на python/perl/etc.
	./test_client -p 1025 -f tests/test01.cmds
report:  report.pdf
.PHONY: doxygen
doxygen: doxygen.cfg src/client-fsm.c src/key-listener.c src/log.c src/maildir.c src/main.c src/opts.c src/protocol.c src/regexp.c src/utils.c 
	doxygen doxygen.cfg
	cp doxygen/latex/*.tex tex/include
.PHONY: clean
clean:
	rm -rf bin/*.o src/*~ include/*~ tex/include/* doxygen/* dot/*.dot; \ 	rm simplest.mk *~; \ 	find tex/ -type f ! -name "*.tex" -exec rm -f {} \;
