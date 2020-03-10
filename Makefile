.PHONY: all clean
.DEFAULT_GOAL := all

LIBS=-lrt -lm -lpmemobj
INCLUDES=-I ../../install/usr/local/include
CFLAGS=-O3 -std=c++11 -w
#CFLAGS=-O0 -std=c++11 -w #-g

output = test 

all: 
	g++ $(CFLAGS) -o bin/test src/pmemlist.cpp $(LIBS)

clean: 
	rm bin/*
