#Gnu makefile for gnu+Linux system

CC=g++

IDIR =include
ODIR=obj
SRCDIR=src
OUTDIR=bin
OUTNAME=OpenGL_app

CXXFLAGS=-std=c++17 -O2 -Wall -Wextra -Wpedantic -Wdouble-promotion -I$(IDIR)

#Yes, there are better ways of doing makefiles but this is a rather small project
_DEPS = graphicus.hpp texwrap.hpp mesh2D.hpp raycaster.hpp
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))


_OBJ = main.o  graphicus.o texwrap.o mesh2D.o raycaster.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

LIBS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lGLEW -lGL

#Create object files
$(ODIR)/%.o: $(SRCDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CXXFLAGS)

#Compile the final program
$(OUTDIR)/$(OUTNAME):	$(OBJ)
	$(CC) -o $@ $^ $(CXXFLAGS) $(LIBS)


.PHONY: clean

clean:
	rm -f $(ODIR)/*.o
	rm -f $(OUTDIR)/$(OUTNAME)
