CPPFLAGS = -g -I. \
	-fPIC \
	-D_REENTRANT \
	-D_PTHREADS \
	-DPOSIX_THREADS \
	-D_GNU_SOURCE \
	-DMEMMGR \
	 -DNAMESPACE=bldgen  \
	-I$(SRCDIR)/config \
	-I$(SRCDIR)/buildgen \
	-I$(SRCDIR)/XLoad \
	-I$(SRCDIR)/jrd \
	-I$(SRCDIR)/include \
	-I$(SRCDIR)/include/gen \
	-I$(SRCDIR)/config \
 

INTDIR = .
SRCDIR = ..
LIBDIR = .
BINDIR = .
INSTALLDIR = $(SRCDIR)/../install

DIRS = $(SRCDIR)/buildgen \
		$(SRCDIR)/XLoad \
		$(SRCDIR)/config \
		$(SRCDIR)/jrd \
		$(SRCDIR)/common \
		$(SRCDIR)/common/classes  \

vpath %.cpp $(DIRS)
vpath %.h $(DIRS)
vpath %.epp $(DIRS)

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
    $(BINDIR)/main.o \
    $(BINDIR)/MakeGen.o \
    $(BINDIR)/ProjectFile.o \
    $(BINDIR)/Stream.o \
    $(BINDIR)/StreamSegment.o \
    $(BINDIR)/Template.o \
    $(BINDIR)/TemplateExpansion.o \
    $(BINDIR)/TemplateValue.o \
    $(BINDIR)/XMLParse.o \


buildgen:	$(MODULES) 
	$(LINK.cpp)  $(CPPFLAGS) -ldl $(MODULES) -L../../install/bin     -lm  \
 -o buildgen

install: buildgen
	cp buildgen $(INSTALLDIR)/bin


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
$(BINDIR)/main.o : main.cpp
$(BINDIR)/MakeGen.o : MakeGen.cpp
$(BINDIR)/ProjectFile.o : ProjectFile.cpp
$(BINDIR)/Stream.o : Stream.cpp
$(BINDIR)/StreamSegment.o : StreamSegment.cpp
$(BINDIR)/Template.o : Template.cpp
$(BINDIR)/TemplateExpansion.o : TemplateExpansion.cpp
$(BINDIR)/TemplateValue.o : TemplateValue.cpp
$(BINDIR)/XMLParse.o : XMLParse.cpp



include makefile.in
