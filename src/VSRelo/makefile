CPPFLAGS = -g -I. \
	-fPIC \
	-D_REENTRANT \
	-D_PTHREADS \
	-DPOSIX_THREADS \
	-D_GNU_SOURCE \
	-DMEMMGR \
	 -DNAMESPACE=VSRelo  \
	-I$(SRCDIR)/config \
	-I$(SRCDIR)/VSRelo \
	-I$(SRCDIR)/XLoad \
	-I$(SRCDIR)/jrd \
	-I$(SRCDIR)/include \
	-I$(SRCDIR)/include/gen \


INTDIR = .
SRCDIR = ..
LIBDIR = .
BINDIR = .
INSTALLDIR = $(SRCDIR)/../builds/posix

DIRS = $(SRCDIR)/VSRelo \
		$(SRCDIR)/XLoad \
		$(SRCDIR)/config \
		$(SRCDIR)/jrd \
		$(SRCDIR)/common \
		$(SRCDIR)/common/classes  \

vpath %.cpp $(DIRS)
vpath %.h $(DIRS)

#CPPOUTPUT = -o $@
$(BINDIR)/%.o : %.cpp
	$(COMPILE.cpp) $< $(OUTPUT_OPTION) $(CPPOUTPUT)

MODULES = \
    $(BINDIR)/AdminException.o \
    $(BINDIR)/Args.o \
    $(BINDIR)/ArgsException.o \
    $(BINDIR)/Element.o \
    $(BINDIR)/InputFile.o \
    $(BINDIR)/InputStream.o \
    $(BINDIR)/JString.o \
    $(BINDIR)/Lex.o \
    $(BINDIR)/PathName.o \
    $(BINDIR)/Relo.o \
    $(BINDIR)/ScanDir.o \
    $(BINDIR)/Stream.o \
    $(BINDIR)/StreamSegment.o \
    $(BINDIR)/vsrelo.o \
    $(BINDIR)/XMLParse.o \


VSRelo:	$(MODULES) 
	$(LINK.cpp)  $(CPPFLAGS) -ldl $(MODULES) -L../../install/bin     -lpthread -lm -ldl -lcrypt \
 -o VSRelo

install: VSRelo
	cp VSRelo $(INSTALLDIR)


clean:
	rm $(MODULES) $(LIBRARY) $(LIB) 

dependencies:
	-make clean > /dev/null 
	-rm *.d > /dev/null 
	make CPPFLAGS='$(CPPFLAGS) -MMD'
	cat *.d > makefile.in


# Dependencies

$(BINDIR)/AdminException.o : AdminException.cpp
$(BINDIR)/Args.o : Args.cpp
$(BINDIR)/ArgsException.o : ArgsException.cpp
$(BINDIR)/Element.o : Element.cpp
$(BINDIR)/InputFile.o : InputFile.cpp
$(BINDIR)/InputStream.o : InputStream.cpp
$(BINDIR)/JString.o : JString.cpp
$(BINDIR)/Lex.o : Lex.cpp
$(BINDIR)/PathName.o : PathName.cpp
$(BINDIR)/Relo.o : Relo.cpp
$(BINDIR)/ScanDir.o : ScanDir.cpp
$(BINDIR)/Stream.o : Stream.cpp
$(BINDIR)/StreamSegment.o : StreamSegment.cpp
$(BINDIR)/vsrelo.o : vsrelo.cpp
$(BINDIR)/XMLParse.o : XMLParse.cpp



include makefile.in
