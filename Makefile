
CXX=g++
CXXFLAGS=-O2 -fno-math-errno
LDFLAGS=-lpng -lSDL2 -lgmp
HEADERS=mj-calc.h mj-adaptive-render.h mj-antialias.h mj-color.h mj-f128.h \
	mj-parseval.h mj-png.h mj-surface.h
PROGS=mj-render mj3-render mj4-render mj5-render mj6-render mj7-render \
	mj8-render mj9-render

.PHONY: all clean
all: $(PROGS)

clean:
	rm -frv $(PROGS)

mj-render: mj-render.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DMJ_MANDELBROT_POWER=2 mj-render.cc -o mj-render

mj3-render: mj-render.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DMJ_MANDELBROT_POWER=3 mj-render.cc -o mj3-render

mj4-render: mj-render.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DMJ_MANDELBROT_POWER=4 mj-render.cc -o mj4-render

mj5-render: mj-render.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DMJ_MANDELBROT_POWER=5 mj-render.cc -o mj5-render

mj6-render: mj-render.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DMJ_MANDELBROT_POWER=6 mj-render.cc -o mj6-render

mj7-render: mj-render.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DMJ_MANDELBROT_POWER=7 mj-render.cc -o mj7-render

mj8-render: mj-render.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DMJ_MANDELBROT_POWER=8 mj-render.cc -o mj8-render

mj9-render: mj-render.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DMJ_MANDELBROT_POWER=9 mj-render.cc -o mj9-render
