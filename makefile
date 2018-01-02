MAKEFLAGS=ks
SRCPATH=src
BINPATH=bin
CC=g++
LIBPATH=/opt/local/lib
HEADPATH=/opt/local/include
FLAGS=-std=gnu++11 -L $(LIBPATH) -I $(HEADPATH) -I src/

all: bin testav

testav: src/main.cpp
	$(CC) -o bin/$@ $(FLAGS)  -lavformat -lavutil -lavcodec $^

bin:
	mkdir bin

run:
	bin/testav info -i sample/test.aac
