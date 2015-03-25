CXXFLAGS = -ggdb -g3 -Wall
CPP_FLAGS = -ggdb -g3 -Wall

ctext: ctext.o
	g++ -ggdb -g3 test.cpp ctext.o -o test -lncursesw
	cat ulysses.txt | ./test

clean:
	rm *.o
