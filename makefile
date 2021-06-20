CXX = clang

CXXFLAGS = -Wall -F /Library/Frameworks
LDFLAGS = -framework SDL2 -F /Library/Frameworks -I /Library/Frameworks/SDL2.framework/Headers

all: clean

clean: echo
	rm main.o

echo: do
	echo //============================// DONE :3

do: main.o
	$(CXX) main.o -o shit8 $(LDFLAGS)

obj/main.o: main.c
	$(CXX) $(CXXFLAGS) -c main.c -o main.o