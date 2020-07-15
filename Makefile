
CXX=g++
CXXFLAGS=-O2 -fno-math-errno
LDFLAGS=-lpng
PROGS=mj-render

.PHONY: all clean
all: $(PROGS)

clean:
	rm -frv $(PROGS)
