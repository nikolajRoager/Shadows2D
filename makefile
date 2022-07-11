#Gnu Makefile for MinGW above 9.2 on Windows. MinGW below 9.2 DOES NOT WORK MAKE SURE IT IS UP TO DATE, AND old MinGW is DELETED
#I recommend using this alongside VisualStudio Code rather than Visual Studio 20**. You might be able to port it to that, but

CC=g++

IDIR =include
ODIR=obj_windows
SRCDIR=src
OUTNAME=OpenGL_app_windows.exe

#I include the headers and library files for the libraries I use in here, because I got annoyed that every OS and every Linux Distro save them slightly differently
LIBDIR = lib_windows


CXXFLAGS=-std=c++17 -O2 -Wall -Wextra -Wpedantic -Wdouble-promotion -I$(IDIR)  -L$(LIBDIR)

#Yes, there are better ways of doing makefiles but this is a rather small project
_DEPS = IO.hpp load_shader.hpp texwrap.hpp SFXwrap.hpp calligraphy.hpp  texwrap.hpp mesh2D.hpp raytracer.hpp
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))


_OBJ = main.o   texwrap.o mesh2D.o raytracer.o IO.o load_shader.o IO_graphics.o IO_audio.o IO_input_devices.o texwrap.o SFXwrap.o calligraphy.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

#Oh looks like you forgot to link glut, and mingw32 ... NO, I DO NOT NEED THOSE FOR THIS TO WORK
LIBS = -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer -lglew32 -lopengl32 
#In all honesty, I have no glue where OpenGL32, including the opengl32 dll is located on my windows system, 


#Create object files
$(ODIR)/%.o: $(SRCDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CXXFLAGS)

#Compile the final program
$(OUTNAME):	$(OBJ)
	$(CC) -o $@ $^ $(CXXFLAGS) $(LIBS)


.PHONY: clean

clean:
	del  .\$(ODIR)\*.o
	del .\$(OUTNAME)
