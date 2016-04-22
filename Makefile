CC := mpic++
FLAGS := -Wall -std=c++0x
TARGET := main

all: $(TARGET)

$(TARGET): main.cpp request.o memorym.o
	$(CC) memorym.o $< -o $@ $(FLAGS)

memorym.o: memorym.cpp request.hpp memorym.hpp 
	$(CC) $< -c -o $@ $(FLAGS)

#request.o: request.cpp request.hpp
#	$(CC) $< -c -o $@ $(FLAGS)

clean:
	rm $(TARGET) *.o
