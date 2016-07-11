CC=gcc
CXX=g++
CFLAGS= -O2
LDFLAGS=-pthread -lpapi

all: bench

.PHONY: all

bench: bench.cpp
	$(CXX) $(CFLAGS) bench.cpp -o bench $(LDFLAGS)

clean:
	rm -f bench
