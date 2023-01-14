PROGNAME = a.out
SRCDIR = src/
OBJDIR = obj/
INCDIR = include/

SRC	= $(wildcard $(SRCDIR)*.cpp)

CC = g++ -std=c++17

WARNFLAGS = -Wall -Wno-deprecated-declarations -Wno-writable-strings
CFLAGS = -g -O3 $(WARNFLAGS) -MD -Iinclude/ -I./ -I/usr/local/include
LDFLAGS =-framework opencl -framework GLUT -framework OpenGL -framework Cocoa -L/usr/local/lib -lGLEW

# Do some substitution to get a list of .o files from the given .cpp files.
OBJFILES = $(patsubst $(SRCDIR)%.cpp, $(OBJDIR)%.o, $(SRC))
INCFILES = $(patsubst $(SRCDIR)%.cpp, $(INCDIR)%.hpp, $(SRC))

.PHONY: all clean

all: $(PROGNAME)

$(PROGNAME): $(OBJFILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)%.o: $(SRCDIR)%.cpp
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -fv $(PROGNAME) $(OBJFILES)
	rm -fv $(OBJDIR)*.d

-include $(OBJFILES:.o=.d)
