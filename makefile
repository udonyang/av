SRCPATH=src
BINPATH=bin
CC=gcc
LIBPATH=/opt/local/lib
HEADPATH=/opt/local/include
FLAGS=-std=gnu++11 -L $(LIBPATH) -I $(HEADPATH) -I .

all: bin testlibffmpeg

testlibffmpeg: src/main.cpp
	$(CC) -o bin/$@ $(FLAGS)  -lavformat $< 

bin:
	mkdir bin
