CXXFLAGS+=-I../include -Wall

all: client

client: main.o
	g++ $(LDFLAGS) -o $@ $^

clean:
	rm -rf client *.o
