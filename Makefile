
CXX=g++
CXXFLAGS=-O2 -fno-math-errno
LDFLAGS=-lpng -lSDL2 -lgmp
PROGS=mj-render

.PHONY: all clean
all: $(PROGS)

clean:
	rm -frv $(PROGS)
