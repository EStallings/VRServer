# makefile for macosx

flags = -Wall -DDEBUG # -O3

% : %.cpp Net.h Model.h
	g++ $< -o $@ ${flags}

all : Test Simple Node

test : Test
	./Test

node : Node
	./Node

simple : Simple
	./Simple

clean:
	rm -f Test Node Simple
