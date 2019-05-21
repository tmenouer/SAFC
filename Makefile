CC=g++
FLAGS=-pthread 
EXEC=allocation

all: $(EXEC)

allocation: 
	$(CC) -o allocation allocation.cpp $(FLAGS)



clean:
	rm -rf allocation
