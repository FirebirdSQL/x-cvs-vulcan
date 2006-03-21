CPPFLAGS = -g -I../jrd -I../include -I../include/gen -D_PTHREADS

MODULES = \
	main.o \
	Events.o

LIBS = \
	-lfirebird \
	-lpthread \
	-L../../install/bin64 \
	-L../../install/bin

events : $(MODULES)
	g++ -g $(MODULES) $(LIBS) -o events

main.o		: main.cpp Events.h
Events.o	: Events.cpp Events.h
