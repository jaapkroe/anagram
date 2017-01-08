CXX=g++ -O3 -std=c++11 -Wall -g

all: anagram
anagram: anagram.cpp

clean:
	rm -f anagram
