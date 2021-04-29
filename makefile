CXX = clang

CXXFLAGS = -Wall -F /Library/Frameworks
LDFLAGS = -framework SDL2 -F /Library/Frameworks -I /Library/Frameworks/SDL2.framework/Headers

all: echo

echo: do
	echo ============================

do: main.o
	$(CXX) main.o -o main $(LDFLAGS)

obj/main.o: main.c
	$(CXX) $(CXXFLAGS) -c main.c -o main.o

clean:
	rm main.o main